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
#include <NameService/Iterator.h>
#include "win32.h"
#include "error2errno.h"

using namespace CosNaming;
using CosNaming::Core::NamingContextRoot;

namespace Nirvana {
namespace Core {

using namespace Windows;

namespace Port {

StringW Dir::get_path () const
{
	return StringW (path (), path_len ());
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
	StringW path = get_path ();
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

StringW Dir::create_path (Name& n, size_t rem_cnt, Name& created) const
{
	StringW path = get_path ();
	while (n.size () > rem_cnt) {
		append_path (path, to_string (n.front ()));
		if (!CreateDirectoryW (path.c_str (), nullptr)) {
			DWORD err = GetLastError ();
			if (ERROR_ALREADY_EXISTS != err)
				throw RuntimeError (error2errno (err));
			if (!(GetFileAttributesW (path.c_str ()) & FILE_ATTRIBUTE_DIRECTORY))
				throw NamingContext::NotFound (NamingContext::NotFoundReason::not_context, std::move (n));
		} else
			created.push_back (std::move (n.front ()));
		n.erase (n.begin ());
	}
	return path;
}

void Dir::remove_created_path (Name& created) const noexcept
{
	size_t folder_cnt = created.size ();
	if (!created.empty ()) {
		StringW path = get_path ();
		for (auto& nc : created) {
			append_path (path, to_string (std::move (nc)));
		}
		do {
			RemoveDirectoryW (path.c_str ());
			path.resize (path.rfind ('\\'));
		} while (--folder_cnt);
	}
}

void Dir::create_link (Name& n, const DirItemId& target, unsigned flags) const
{
	// Create symbolic link
	Name created;
	StringW path = create_path (n, 1, created);

	try {
		append_path (path, to_string (std::move (n.back ())));

		if (flags & FLAG_REBIND) {
			flags &= ~FLAG_REBIND;

			DWORD att = GetFileAttributesW (path.c_str ());
			if (0xFFFFFFFF != att)
				unbind (path.c_str (), att);
		}

		if (!CreateSymbolicLinkW (path.c_str (), FileSystem::id_to_path (target), flags))
			throw RuntimeError (error2errno (GetLastError ()));

	} catch (...) {
		remove_created_path (created);
		throw;
	}
}

void Dir::unbind (const WinWChar* path, uint32_t att)
{
	BOOL ok;
	if (FILE_ATTRIBUTE_DIRECTORY & att)
		ok = RemoveDirectoryW (path);
	else
		ok = DeleteFileW (path);
	if (!ok)
		throw_last_error (); 
}

void Dir::unbind (Name& n) const
{
	StringW path = check_path (n, 1);
	append_path (path, to_string (n.back ()));

	DWORD att = GetFileAttributesW (path.c_str ());
	if (0xFFFFFFFF == att)
		throw NamingContext::NotFound (NamingContext::NotFoundReason::missing_node, std::move (n));

	unbind (path.c_str (), att);
}

DirItemId Dir::create_dir (Name& n) const
{
	Name created;
	StringW path = create_path (n, 1, created);

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
		remove_created_path (created);
		throw;
	}
}

DirItemId Dir::get_new_file_id (Name& n) const
{
	// Create id for parent path
	DirItemId id = FileSystem::path_to_id (check_path (n, 1).c_str (), Nirvana::DirItem::FileType::regular);

	// Append name
	StringW name = to_wstring (to_string (n.back ()));
	size_t parent_size = id.size () - sizeof (WinWChar);
	id.resize (id.size () + name.size () + 1);
	const WinWChar* src = name.data ();
	std::copy (src, src + name.size () + 1, (WinWChar*)(id.data () + parent_size));

	return id;
}

class Dir::Iterator : public CosNaming::Core::Iterator
{
public:
	Iterator (const WinWChar* pattern) :
		handle_ (INVALID_HANDLE_VALUE)
	{
		handle_ = FindFirstFileExW (pattern, FindExInfoBasic, &data_, FindExSearchNameMatch, nullptr, 0);
		if (INVALID_HANDLE_VALUE == handle_)
			throw_last_error ();
	}

	~Iterator ()
	{
		if (INVALID_HANDLE_VALUE != handle_)
			FindClose (handle_);
	}

	virtual bool end () const noexcept override
	{
		return INVALID_HANDLE_VALUE == handle_;
	}

	virtual bool next_one (Binding& b) override
	{
		if (INVALID_HANDLE_VALUE != handle_) {
			wide_to_utf8 ((const WinWChar*)data_.cFileName, b.name);

			if (data_.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				b.type = BindingType::ncontext;
			else
				b.type = BindingType::nobject;

			if (!FindNextFileW (handle_, &data_)) {
				FindClose (handle_);
				handle_ = INVALID_HANDLE_VALUE;
			}
			return true;
		}
		return false;
	}

private:
	WIN32_FIND_DATAW data_;
	HANDLE handle_;
};

}
}
}
