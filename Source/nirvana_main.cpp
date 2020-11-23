#include "SchedulerMaster.h"
#include "SchedulerSlave.h"
#include "shutdown.h"
#include "initterm.h"
#include "Console.h"
#include <StartupProt.h>
#include <StartupSys.h>

#define entry_point nirvana_startup
#define main nirvana_main
#include "startup.h"

using namespace Nirvana;
using namespace Nirvana::Core;
using namespace Nirvana::Core::Windows;

inline
int run (int argc, char* argv [])
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
							Console::write ("System is already running.\n");
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
							Console::write ("Invalid command line.\n");
							return -1;
						} else {
							StartupProt startup (argc, argv);
							if (!SchedulerSlave (sys_process_id, semaphore).run (startup, startup_deadline)) {
								Console::write ("System is not running.\n");
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
							Console::write ("System is not running.\n");
							return -1;
						}
				}
			}
		}

		StartupProt startup (argc, argv);
		if (!SchedulerSlave ().run (startup, StartupProt::default_deadline ())) {
			Console::write ("System is not running.\n");
			return -1;
		} else {
			startup.check ();
			return startup.ret ();
		}
	} catch (const std::exception& ex) {
		Console::write (ex.what ());
		Console::write ("\n");
		return -1;
	}
}

extern "C" int __cdecl main (int argc, char* argv [], char** envp)
{
	int ret = run (argc, argv);
	Nirvana::Core::Windows::terminate ();
	return ret;
}
