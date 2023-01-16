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
#include "app_data.h"
#include <shlobj_core.h>
#include <Shlwapi.h>
#include <algorithm>

namespace Nirvana {
namespace Core {
namespace Windows {

uint32_t sys_process_id;

long get_app_data_path (WCHAR* path, bool create) NIRVANA_NOEXCEPT
{
	HRESULT hr = SHGetFolderPathW (NULL, CSIDL_COMMON_APPDATA, NULL, 0, path);
	if (S_OK != hr)
		return hr;
	WCHAR* p = path + wcslen (path);
	static const WCHAR nirvana [] = WINWCS ("\\Nirvana");
	for (size_t i = 0; i < 2; ++i) {
		p = std::copy (nirvana, nirvana + std::size (nirvana), p) - 1;
		if (create && !CreateDirectoryW (path, nullptr)) {
			DWORD err = GetLastError ();
			if (ERROR_ALREADY_EXISTS != err)
				return HRESULT_FROM_WIN32 (err);
		}
	}
	static const WCHAR term [] = L"\\";
	std::copy (term, term + 2, p);
	return 0;
}

HANDLE open_sysdomainid (bool write)
{
	WCHAR path [MAX_PATH + 1];
	HRESULT hr = get_app_data_path (path, write);
	if (S_OK != hr)
		throw_INITIALIZE (hr);
	static const WCHAR sysdomainid [] = WINWCS ("sysdomainid");
	std::copy (sysdomainid, sysdomainid + std::size (sysdomainid), path + wcslen (path));
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
		flags = FILE_ATTRIBUTE_TEMPORARY;
	}
	HANDLE hf = CreateFileW (path, access, share, nullptr, disposition, flags, nullptr);
	if (INVALID_HANDLE_VALUE == hf) {
		DWORD err = GetLastError ();
		if (write) {
			if (ERROR_SHARING_VIOLATION == err)
				return hf;
		} else if (ERROR_FILE_NOT_FOUND == err)
			return hf;

		throw_INITIALIZE (HRESULT_FROM_WIN32 (err));
	}
	return hf;
}

bool get_sys_process_id ()
{
	HANDLE sysdomainid = open_sysdomainid (false);
	if (INVALID_HANDLE_VALUE == sysdomainid)
		return false;
	DWORD cbread = 0;
	bool OK = ReadFile (sysdomainid, &sys_process_id, sizeof (DWORD), &cbread, nullptr)
		&& sizeof (DWORD) == cbread;
	CloseHandle (sysdomainid);
	return OK;
}

}
}
}
