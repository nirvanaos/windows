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
#include "ErrConsole.h"
#include "win32.h"
#include <string.h>

namespace Nirvana {
namespace Core {
namespace Windows {

HANDLE ErrConsole::handle_;
bool ErrConsole::allocated_;

HANDLE ErrConsole::attach () NIRVANA_NOEXCEPT
{
	if (!handle_) {
		if (!IsDebuggerPresent ()) {
			if (!AttachConsole (ATTACH_PARENT_PROCESS)) {
				AllocConsole ();
				allocated_ = true;
			}
		} else {
			AllocConsole ();
			allocated_ = true;
		}
		handle_ = GetStdHandle (STD_ERROR_HANDLE);
	}

	return handle_;
}

ErrConsole::ErrConsole ()
{
	attach ();
}

ErrConsole::~ErrConsole ()
{
	if (handle_ && allocated_) {
		operator << ("Press any key to close this window...\n");
		HANDLE h = GetStdHandle (STD_INPUT_HANDLE);
		INPUT_RECORD input;
		DWORD cnt;
		while (ReadConsoleInput (h, &input, 1, &cnt)) {
			if (KEY_EVENT == input.EventType && input.Event.KeyEvent.uChar.UnicodeChar)
				break;
		}
	}
}

const ErrConsole& ErrConsole::operator << (const char* text) const NIRVANA_NOEXCEPT
{
	write (text, strlen (text));
	return *this;
}

void ErrConsole::write (const char* text, size_t len) const NIRVANA_NOEXCEPT
{
	if (handle_) {
		DWORD cb;
		WriteFile (handle_, text, (DWORD)len, &cb, nullptr);
	}
}

}
}
}
