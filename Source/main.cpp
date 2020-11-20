#include <StartupSys.h>
#include <StartupProt.h>
#include "SchedulerMaster.h"
#include "SchedulerSlave.h"
#include "Message.h"
#include "Mailslot.h"
#include "MailslotName.h"
#include <iostream>

using namespace std;
using namespace Nirvana;
using namespace Nirvana::Core;
using namespace Nirvana::Core::Windows;

bool shutdown ()
{
	Mailslot ms;
	if (ms.open (MailslotName (0))) {
		try {
			ms.send (Message::Shutdown ());
			return true;
		} catch (...) {
		}
	}
	return false;
}

int main (int argc, char* argv [])
{
	++argv;
	if (--argc > 0) {
		const char* arg = *argv;
		if ('-' == arg [0]) {
			switch (arg [1]) {

				case 's': {
					--argc;
					++argv;
					StartupSys startup (argc, argv);
					if (!SchedulerMaster ().run (startup, StartupSys::default_deadline ())) {
						cout << "System is already running." << endl;
						return -1;
					} else
						return 0;
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
						cout << "Invalid command line." << endl;
						return -1;
					} else {
						StartupProt startup (argc, argv);
						if (!SchedulerSlave (sys_process_id, semaphore).run (startup, startup_deadline)) {
							cout << "System is not running." << endl;
							return -1;
						} else
							return 0;
					}
					break;
				}

				case 'd':
					if (shutdown ())
						return 0;
					else {
						cout << "System is not running." << endl;
						return -1;
					}
			}
		}
	}

	StartupProt startup (argc, argv);
	if (!SchedulerSlave ().run (startup, StartupProt::default_deadline ())) {
		cout << "System is not running." << endl;
		return -1;
	}

	return 0;
}
