#include "SchedulerMaster.h"
#include <GTestSys.h>
#include <iostream>

namespace Nirvana {
namespace Core {
namespace Windows {

inline
int gtest_sys (int argc, char* argv []) NIRVANA_NOEXCEPT
{
	try {
		Test::GTestSys startup (argc, argv);
		if (!SchedulerMaster ().run (startup, startup.default_deadline ())) {
			std::cout << "System is already running." << std::endl;
			return -1;
		} else {
			startup.check ();
			return startup.ret ();
		}
	} catch (const std::exception& ex) {
		std::cout << ex.what () << std::endl;
		return -1;
	}
}

}
}
}

#define MAIN gtest_sys
#include "startup.h"
