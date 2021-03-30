/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
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
#include "Console.h"
#include <StartupProt.h>
#include <StartupSys.h>
#include "start.h"

namespace Nirvana {
namespace Core {
namespace Windows {

inline
int nirvana (int argc, char* argv []) NIRVANA_NOEXCEPT
{
	try {
		++argv;
		if (--argc > 0) {
			const char* arg = *argv;
			if ('-' == arg [0]) {
				switch (arg [1]) {

					case 's': {
						--argc;
						++argv;
						StartupSys startup (argc, argv);
						if (!SchedulerMaster ().run (startup, startup.default_deadline ())) {
							Console () << "System is already running.\n";
							return -1;
						} else {
							startup.check ();
							return startup.ret ();
						}
					}

					case 'p': {
						uint32_t sys_process_id = 0;
						uint32_t semaphore = 0;
						DeadlineTime startup_deadline = StartupProt::default_deadline ();
						if (--argc > 0) {
							++argv;
							char* end;
							sys_process_id = strtoul (*argv, &end, 16);
							if (!*end && --argc > 0) {
								++argv;
								semaphore = strtoul (*argv, &end, 16);
								if (*end || argc > 1)
									semaphore = 0;
								else if (argc > 0) {
									++argv;
									startup_deadline = strtoull (*argv, &end, 16);
									if (end)
										semaphore = 0;
								}
							}
						}

						if (!semaphore) {
							Console () << "Invalid command line.\n";
							return -1;
						} else {
							StartupProt startup (argc, argv);
							if (!SchedulerSlave (sys_process_id, semaphore).run (startup, startup_deadline)) {
								Console () << "System is not running.\n";
								return -1;
							} else {
								startup.check ();
								return startup.ret ();
							}
						}
						break;
					}

					case 'd':
						if (shutdown ())
							return 0;
						else {
							Console () << "System is not running.\n";
							return -1;
						}
				}
			}
		}

		StartupProt startup (argc, argv);
		if (!SchedulerSlave ().run (startup, StartupProt::default_deadline ())) {
			Console () << "System is not running.\n";
			return -1;
		} else {
			startup.check ();
			return startup.ret ();
		}
	} catch (const std::exception& ex) {
		Console () << ex.what () << '\n';
		return -1;
	}
}

}
}
}

NIRVANA_MAIN(nirvana)
