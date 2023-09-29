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
#include "pch.h"
#include "../Port/SysDomain.h"
#include "win32.h"
#include "WinWChar.h"
#include "error2errno.h"

namespace Nirvana {
namespace Core {
using namespace Windows;
namespace Port {

uint32_t SysDomain::create_prot_domain (unsigned platform, const IDL::String& host, unsigned port)
{
	WinWChar path [MAX_PATH + 1];
	DWORD cc = GetModuleFileNameW (nullptr, path, std::size (path));
	if (!cc || cc == std::size (path))
		throw_last_error ();

	WinWChar* name = path + cc;
	while (name > path) {
		if ('\\' == *--name)
			break;
	}
	++name;

	const WinWChar x86 [] = WINWCS ("x86");
	if (platform != PLATFORM) {
		const WinWChar* folder;
		if (platform == PLATFORM_I386)
			folder = x86;
		else
			throw_BAD_PARAM ();

		WinWChar exe [16];
		wcscpy (exe, name);
		name = std::copy (folder, folder + wcslen (folder), name);
		*(name++) = '\\';
		wcscpy (name, exe);
	}

	StringW cmd_line = path;
	StringW dir (path, name - 1);
	STARTUPINFOW si;
	zero (si);
	PROCESS_INFORMATION pi;
	if (!CreateProcessW (path, const_cast <WinWChar*> (cmd_line.data ()), nullptr, nullptr, false, 0,
		nullptr, dir.c_str (), &si, &pi))
		throw_last_error ();

	CloseHandle (pi.hProcess);
	CloseHandle (pi.hThread);

	return pi.dwProcessId;
}

}
}
}
