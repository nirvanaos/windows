#include "SchedulerMaster.h"
#include "initterm.h"
#include "Console.h"
#include <GTestSys.h>

namespace Nirvana {
namespace Core {
namespace Windows {

inline
int gtest_sys (int argc, char* argv []) NIRVANA_NOEXCEPT
{
	try {
		Test::GTestSys startup (argc, argv);
		if (!SchedulerMaster ().run (startup, startup.default_deadline ())) {
			Console::write ("System is already running.\n");
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

}
}
}

#define MAIN gtest_sys
#include "startup.h"
