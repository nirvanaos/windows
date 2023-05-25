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
#include "SchedulerMaster.h"
#include "SchedulerSlave.h"
#include "shutdown.h"
#include "ErrConsole.h"
#include <StartupProt.h>
#include <StartupSys.h>
#include "start.h"

namespace Nirvana {
namespace Core {
namespace Windows {

inline
int nirvana (int argc, char* argv [], char* envp []) NIRVANA_NOEXCEPT
{
	try {
		if (argc > 1) {
			const char* arg = argv [1];
			if ('-' == arg [0]) {
				switch (arg [1]) {

				case 's':
					if (BUILD_NO_SYS_DOMAIN) {
						ErrConsole () << "Wrong platform. Use 64-bit version.\n";
						return -1;
					} else {
						std::copy (argv + 2, argv + argc, argv + 1);
						--argc;
						StartupSys startup (argc, argv, envp);
						if (!SchedulerMaster ().run (startup)) {
							ErrConsole () << "System is already running.\n";
							return -1;
						} else {
							startup.check ();
							return startup.ret ();
						}
					}
/*
					case 'p': {
						uint32_t sys_process_id = 0;
						uint32_t semaphore = 0;
						DeadlineTime startup_deadline = StartupProt::default_deadline ();
						if (argc >= 4 && argc <= 5) {
							char* end;
							sys_process_id = strtoul (argv [2], &end, 16);
							if (!*end) {
								semaphore = strtoul (argv [3], &end, 16);
								if (*end)
									semaphore = 0;
								else if (argc >= 5) {
									startup_deadline = strtoull (argv [4], &end, 16);
									if (*end)
										semaphore = 0;
								}
							}
						}

						if (!semaphore) {
							ErrConsole () << "Invalid command line.\n";
							return -1;
						} else {
							StartupProt startup (1, argv, envp);
							if (!SchedulerSlave (sys_process_id, semaphore).run (startup, startup_deadline)) {
								ErrConsole () << "System is not running.\n";
								return -1;
							} else {
								startup.check ();
								return startup.ret ();
							}
						}
						break;
					}
*/
					case 'd':
						if (shutdown ())
							return 0;
						else {
							ErrConsole () << "System is not running.\n";
							return -1;
						}
				}
			}
		}

		StartupProt startup (argc, argv, envp);
		if (!SchedulerSlave ().run (startup, StartupProt::default_deadline ())) {
			ErrConsole () << "System is not running.\n";
			return -1;
		} else {
			startup.check ();
			return startup.ret ();
		}
	} catch (const CORBA::SystemException& ex) {
		char buf [_MAX_ITOSTR_BASE16_COUNT];
		_itoa (ex.minor (), buf, 16);
		ErrConsole () << ex._name () << "(0x" << buf << ")\n";
	} catch (const std::exception& ex) {
		ErrConsole () << ex.what () << '\n';
	}
	return -1;
}

}
}
}

NIRVANA_MAIN(nirvana)
