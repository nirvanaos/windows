/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include "../Port/Dir.h"
#include "DirIterator.h"
#include "win32.h"
#include "error2errno.h"

using namespace CosNaming;
using CosNaming::Core::NamingContextRoot;

namespace Nirvana {
namespace Core {

using namespace Windows;

namespace Port {

Dir::Dir (StringW&& path) :
	Base (std::move (path)),
	type_ (Nirvana::DirItem::FileType::none)
{}

Dir::Dir () :
	type_ (Nirvana::DirItem::FileType::directory)
{}

Nirvana::DirItem::FileType Dir::type () const noexcept
{
	if (Nirvana::DirItem::FileType::none == type_) {
		HANDLE h = get_handle ();
		if (INVALID_HANDLE_VALUE == h)
			type_ = Nirvana::DirItem::FileType::not_found;
		else
			type_ = Nirvana::DirItem::FileType::directory;
	}

	return type_;
}

StringW Dir::get_path (CosNaming::Name& n) const
{
	assert (!n.empty ());
	return path ();
}

StringW Dir::to_wstring (Istring name)
{
	StringW wname;
	utf8_to_wide (name, wname);
	for (WinWChar c : wname) {
		if (c < 32 || wcschr (WINWCS ("<>:\"/\\|?*"), c))
			throw CosNaming::NamingContext::InvalidName ();
	}
	return wname;
}

void Dir::append_path (StringW& path, Istring name)
{
	StringW wname = to_wstring (name);
	path.reserve (path.size () + 1 + wname.size ());
	path += '\\';
	path += wname;
}

StringW Dir::check_path (Name& n, size_t rem_cnt) const
{
	StringW path = get_path (n);
	while (n.size () > rem_cnt) {
		append_path (path, to_string (n.front ()));
		DWORD att = GetFileAttributesW (path.c_str ());
		NamingContext::NotFoundReason reason = NamingContext::NotFoundReason::not_object;
		if (0xFFFFFFFF == att)
			reason = NamingContext::NotFoundReason::missing_node;
		else if (!(FILE_ATTRIBUTE_DIRECTORY & att))
			reason = NamingContext::NotFoundReason::not_context;
		if (reason != NamingContext::NotFoundReason::not_object)
			throw CosNaming::NamingContext::NotFound (reason, std::move (n));
		n.erase (n.begin ());
	}
	return path;
}

StringW Dir::create_path (Name& n, size_t rem_cnt, size_t& created_begin) const
{
	StringW path = get_path (n);
	size_t created_start = 0;
	while (n.size () > rem_cnt) {
		size_t len = path.size ();
		append_path (path, to_string (n.front ()));
		if (!CreateDirectoryW (path.c_str (), nullptr)) {
			DWORD err = GetLastError ();
			if (ERROR_ALREADY_EXISTS != err)
				throw RuntimeError (error2errno (err));
			if (!(GetFileAttributesW (path.c_str ()) & FILE_ATTRIBUTE_DIRECTORY))
				throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, std::move (n));
		} else if (!created_start)
			created_start = len;
		n.erase (n.begin ());
	}
	created_begin = created_start;
	return path;
}

void Dir::remove_created_path (Windows::StringW& path, size_t created_begin) const noexcept
{
	while (path.size () > created_begin) {
		RemoveDirectoryW (path.c_str ());
		path.resize (path.rfind ('\\'));
	}
}

void Dir::create_link (Name& n, const DirItemId& target, unsigned flags) const
{
	// Create symbolic link
	size_t created_begin;
	StringW path = create_path (n, 1, created_begin);

	try {
		append_path (path, to_string (std::move (n.back ())));

		if (flags & FLAG_REBIND) {
			flags &= ~FLAG_REBIND;

			DWORD att = GetFileAttributesW (path.c_str ());
			if (0xFFFFFFFF != att)
				unlink (path.c_str (), att);
		}

		if (!CreateSymbolicLinkW (path.c_str (), FileSystem::id_to_path (target), flags))
			throw RuntimeError (error2errno (GetLastError ()));

	} catch (...) {
		remove_created_path (path, created_begin);
		throw;
	}
}

void Dir::unlink (const WinWChar* path, uint32_t att)
{
	BOOL ok;
	if (FILE_ATTRIBUTE_DIRECTORY & att)
		ok = RemoveDirectoryW (path);
	else
		ok = DeleteFileW (path);
	if (!ok)
		throw_last_error (); 
}

void Dir::unlink (Name& n) const
{
	StringW path = check_path (n, 1);
	append_path (path, to_string (n.back ()));

	DWORD att = GetFileAttributesW (path.c_str ());
	if (0xFFFFFFFF == att)
		throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, std::move (n));

	unlink (path.c_str (), att);
}

DirItemId Dir::create_dir (Name& n) const
{
	size_t created_begin;
	StringW path = create_path (n, 1, created_begin);

	try {
		append_path (path, to_string (std::move (n.back ())));

		if (!CreateDirectoryW (path.c_str (), nullptr)) {
			DWORD err = GetLastError ();
			if (ERROR_ALREADY_EXISTS == err)
				throw NamingContext::AlreadyBound ();
			else
				throw RuntimeError (error2errno (err));
		}

		return FileSystem::path_to_id (path.c_str (), Nirvana::DirItem::FileType::directory);

	} catch (...) {
		remove_created_path (path, created_begin);
		throw;
	}
}

DirItemId Dir::get_new_file_id (Name& n) const
{
	// Create id for parent path
	DirItemId id = FileSystem::path_to_id (check_path (n, 1).c_str (), Nirvana::DirItem::FileType::regular);

	// Append file name
	StringW name = to_wstring (to_string (n.back ()));
	size_t parent_size = id.size () - sizeof (WinWChar); // Without the zero terminator
	size_t name_size = name.size ();
	id.resize (id.size () + name_size * sizeof (WinWChar) + sizeof (WinWChar));
	const WinWChar* src = name.data ();
	WinWChar* dst = (WinWChar*)(id.data () + parent_size);
	assert (!*dst);
	*(dst++) = '\\';
	std::copy (src, src + name_size + 1, dst);

	return id;
}

StringW Dir::get_pattern () const
{
	return path () + WINWCS ("\\*.*");
}

std::unique_ptr <CosNaming::Core::Iterator> Dir::make_iterator () const
{
	return std::make_unique <DirIterator> (get_pattern ().c_str ());
}

}
}
}
