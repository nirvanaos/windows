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
#include "SchedulerBase.h"
#include <shlobj_core.h>
#include <Shlwapi.h>

namespace Nirvana {
namespace Core {
namespace Windows {

uint32_t sys_process_id;

HANDLE open_sysdomainid (bool write) NIRVANA_NOEXCEPT
{
	WCHAR path [MAX_PATH];
	if (S_OK != SHGetFolderPathW (NULL, CSIDL_COMMON_APPDATA, NULL, 0, path))
		return INVALID_HANDLE_VALUE;
	for (size_t i = 0; i < 2; ++i) {
		PathAppendW (path, L"\\Nirvana");
		if (write && !CreateDirectoryW (path, nullptr) && ERROR_ALREADY_EXISTS != GetLastError ())
			return INVALID_HANDLE_VALUE;
	}
	PathAppendW (path, L"\\sysdomainid");
	DWORD access, share, disposition, flags;
	if (write) {
		access = GENERIC_WRITE;
		share = FILE_SHARE_READ;
		disposition = CREATE_ALWAYS;
		flags = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
	} else {
		access = GENERIC_READ;
		share = FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE;
		disposition = OPEN_EXISTING;
		flags = FILE_ATTRIBUTE_NORMAL;
	}
	return CreateFileW (path, access, share, nullptr, disposition, flags, nullptr);
}

bool get_sys_process_id () NIRVANA_NOEXCEPT
{
	HANDLE sysdomainid = open_sysdomainid (false);
	if (INVALID_HANDLE_VALUE == sysdomainid)
		return false;
	DWORD cbread = 0;
	BOOL OK = ReadFile (sysdomainid, &sys_process_id, sizeof (DWORD), &cbread, nullptr);
	CloseHandle (sysdomainid);
	return OK && sizeof (DWORD) == cbread;
}

}
}
}
