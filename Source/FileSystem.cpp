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
#include "../Port/FileSystem.h"
#include "app_data.h"
#include "error2errno.h"
#include <NameService/Dir.h>
#include <NameService/File.h>
#include "Dir_var.h"
#include "Dir_mnt.h"
#include <ShlObj.h>

namespace Nirvana {
namespace Core {

using namespace Windows;

namespace Port {

const FileSystem::Root FileSystem::roots_ [] = {
	{ "etc", get_app_data_dir },
	{ "home", get_home },
	{ "mnt", get_mnt },
	{ "var", get_var }
};

Roots FileSystem::get_roots ()
{
	Roots roots;
	roots.reserve (std::size (roots_));
	for (const Root* p = roots_; p != std::end (roots_); ++p) {
		roots.push_back ({ p->dir, p->factory });
	}
	return roots;
}

DirItemId FileSystem::get_app_data_dir (const IDL::String& name, bool& may_cache)
{
	if (!name.size () || name.size () > 3)
		throw CORBA::BAD_PARAM ();

	WinWChar wname [4] = { 0 };
	std::copy (name.begin (), name.end (), wname);

	WinWChar path [MAX_PATH + 1];
	get_app_data_folder (path, std::size (path), wname, false);

	may_cache = true;
	return path_to_id (path, Nirvana::DirItem::FileType::directory);
}

StringW FileSystem::get_app_data_dir (const WinWChar* name)
{
	WinWChar path [MAX_PATH + 1];
	size_t cc = get_app_data_folder (path, std::size (path), name, false);
	return StringW (path, cc);
}

DirItemId FileSystem::get_var (const IDL::String&, bool& may_cache)
{
	may_cache = true;
	return make_special_id (SpecialDir::var);
}

DirItemId FileSystem::get_mnt (const IDL::String&, bool& may_cache)
{
	may_cache = true;
	return make_special_id (SpecialDir::mnt);
}

DirItemId FileSystem::get_home (const IDL::String&, bool& may_cache)
{
	may_cache = false;

	WinWChar path [MAX_PATH];
	HRESULT result = SHGetFolderPathW (NULL, CSIDL_PROFILE, NULL, 0, path);
	if (SUCCEEDED (result))
		return path_to_id (path, Nirvana::DirItem::FileType::directory);
	else
		throw CORBA::UNKNOWN ();
}

DirItemId FileSystem::path_to_id (const WinWChar* path, Nirvana::DirItem::FileType type)
{
	DirItemId id;

	assert (path [0] && path [1]);
	if (!path [2]) {

		// Drive
		assert ('A' <= path [0] && path [0] <= 'Z');
		assert (':' == path [1]);
		if (Nirvana::DirItem::FileType::unknown == type)
			type = Nirvana::DirItem::FileType::directory;
		assert (type == Nirvana::DirItem::FileType::directory);
		id.resize (4 * sizeof (WinWChar));
		WinWChar* dst = (WinWChar*)id.data () + 1;
		dst [0] = path [0];
		dst [1] = path [1];
		dst [2] = 0;

	} else {

		// Get final path name
		HANDLE h = CreateFileW (path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, nullptr);
		if (INVALID_HANDLE_VALUE == h)
			throw_last_error ();

		try {
			DWORD cc = GetFinalPathNameByHandleW (h, nullptr, 0, 0);
			if (!cc)
				throw_last_error ();
			id.resize ((cc + 1) * sizeof (WinWChar));
			verify (GetFinalPathNameByHandleW (h, (WinWChar*)(id.data () + 2), cc, 0));

			if (Nirvana::DirItem::FileType::unknown == type) {
				BY_HANDLE_FILE_INFORMATION bhfi;
				if (!GetFileInformationByHandle (h, &bhfi))
					throw_last_error ();
				if (bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					type = Nirvana::DirItem::FileType::directory;
				else
					type = Nirvana::DirItem::FileType::regular;
			}
		} catch (...) {
			CloseHandle (h);
			throw;
		}
		CloseHandle (h);
	}

	// Save file type as first WinWChar
	*((WinWChar*)id.data ()) = (WinWChar)type;
	return id;
}

DirItemId FileSystem::make_special_id (SpecialDir dir)
{
	DirItemId id (4);
	WinWChar* p = (WinWChar*)id.data ();
	*(p++) = (WinWChar)Nirvana::DirItem::FileType::directory;
	*(p++) = (WinWChar)dir;
	return id;
}

PortableServer::ServantBase::_ref_type FileSystem::incarnate (const DirItemId& id)
{
	if (get_item_type (id) == Nirvana::DirItem::FileType::directory) {
		switch (is_special_dir (id)) {
		case SpecialDir::var:
			return CORBA::make_reference <Windows::Dir_var> (get_app_data_dir (WINWCS("var")));

		case SpecialDir::mnt:
			return CORBA::make_reference <Windows::Dir_mnt> ();
		
		default:
			return CORBA::make_reference <Nirvana::Core::Dir> (make_path (id));
		}

	} else
		return CORBA::make_reference <Nirvana::Core::File> (make_path (id));
}

}
}
}
