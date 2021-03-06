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
#include "../Port/Debugger.h"
#include "WinWChar.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

void Debugger::output_debug_string (const char* msg)
{
	Windows::StringW ws;
	Windows::utf8_to_ucs16 (msg, ws);
	if (IsDebuggerPresent ())
		OutputDebugStringW (ws.c_str ());
	else {
		if (!AttachConsole (ATTACH_PARENT_PROCESS))
			AllocConsole ();
		DWORD written;
		WriteConsoleW (GetStdHandle (STD_ERROR_HANDLE), ws.data (), (DWORD)ws.size (), &written, nullptr);
	}
}

bool Debugger::debug_break ()
{
	if (IsDebuggerPresent ()) {
		__debugbreak ();
		return true;
	}
	return false;
}

}
}
}
