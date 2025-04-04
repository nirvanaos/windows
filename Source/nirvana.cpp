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
#include "initialize.h"
#include "CmdLineParser.h"
#include "SchedulerMaster.h"
#include "SchedulerSlave.h"
#include "shutdown.h"
#include "ErrConsole.h"
#include "DebugLog.h"
#include <StartupProt.h>
#include <StartupSys.h>
#include <initterm.h>

namespace Nirvana {
namespace Core {
namespace Windows {

static void swallow_arg (int& argc, char* argv [])
{
	real_copy (argv + 2, argv + argc, argv + 1);
	--argc;
}

inline
int nirvana (int argc, char* argv [], char* envp [])
{
	bool system = false;
	while (argc > 1 && '-' == *argv [1]) {
		char cmd = argv [1][1];
		switch (cmd) {

		case 't':
			DebugLog::trace_ = true;
			swallow_arg (argc, argv);
			break;

		case 's':
			system = true;
			swallow_arg (argc, argv);
			break;

		case 'd':
		case 'f':
			if (shutdown ('f' == cmd))
				return 0;
			else {
				ErrConsole () << "System is not running.\n";
				return -1;
			}

		default:
			goto start;
		}
	}

start:

	if (system) {
		if (BUILD_NO_SYS_DOMAIN) {
			ErrConsole () << "Wrong platform. Use 64-bit version.\n";
			return -1;
		} else {
			StartupSys startup (argc, argv, envp);
			if (!SchedulerMaster ().run (startup)) {
				ErrConsole () << "System is already running.\n";
				return -1;
			} else {
				startup.check ();
				return startup.ret ();
			}
		}
	} else {
		StartupProt startup (argc, argv, envp);
		if (!SchedulerSlave ().run (startup, StartupProt::default_deadline ())) {
			ErrConsole () << "System is not running.\n";
			return -1;
		} else {
			startup.check ();
			return startup.ret ();
		}
	}
}

}
}
}

extern "C" DWORD WinMainCRTStartup (void);

extern "C" DWORD nirvana_startup (void)
{
	if (!Nirvana::Core::Windows::initialize_windows ())
		return -1;
	return WinMainCRTStartup ();
}

int CALLBACK WinMain (HINSTANCE, HINSTANCE, LPSTR, int)
{
	try {
		Nirvana::Core::initialize0 ();
		Nirvana::Core::Windows::CmdLineParser cmdline;
		return Nirvana::Core::Windows::nirvana (cmdline.argc (), cmdline.argv (), cmdline.envp ());
	} catch (const std::exception& ex) {
		Nirvana::Core::Windows::ErrConsole () << ex.what () << '\n';
	}
	return -1;
}
