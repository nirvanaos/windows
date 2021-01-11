#include <GTestSys.h>
#include <iostream>
#include "SchedulerMaster.h"
#include "start.h"

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

NIRVANA_MAIN(gtest_sys)
