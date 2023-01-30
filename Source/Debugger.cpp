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
#include "DebugLog.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

void Debugger::output_debug_string (const char* msg)
{
	// Use shared string to avoid possible infinite recursion in assertions.
	if (IsDebuggerPresent ()) {
		Windows::SharedStringW ws;
		utf8_to_wide (msg, ws);
		OutputDebugStringW (ws.c_str ());
	} else
		Windows::DebugLog () << msg;
}

bool Debugger::debug_break ()
{
	if (IsDebuggerPresent ()) {
		__debugbreak ();
		return true;
	}

	// If we are out of the execution domain, for example, in the postman thread,
	// __debugbreak will cause unhandled exception and stack trace.
	Runnable* r = nullptr;
	Core::Thread* th = Thread::current ();
	if (th) {
		ExecDomain* ed = th->exec_domain ();
		if (ed)
			r = ed->runnable ();
	}
	if (!r) {
		__debugbreak ();
		return true;
	}

	return false;
}

}
}
}
