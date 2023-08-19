/// \file
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
#ifndef NIRVANA_CORE_PORT_DEBUGGER_H_
#define NIRVANA_CORE_PORT_DEBUGGER_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/System.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// \brief Debugging support.
class Debugger
{
public:
	/// \brief Prints message to debug output.
	/// \param msg The message.
	static void output_debug_string (System::DebugEvent level, const char* msg);

	/// \brief Debug break.
	/// 
	/// \returns If debugger is available, performs debug break and returns `true`.
	/// Otherwise returns `false`;
	static bool debug_break ();

	/// \brief Determines whether the calling process is being debugged by a user-mode debugger.
	/// 
	/// \returns If the current process is running in the context of a debugger, the return value
	///          is `true`.
	static bool is_debugger_present () noexcept;
};

}
}
}

#endif
