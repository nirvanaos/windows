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
#include <GTestSys.h>
#include <iostream>
#include "SchedulerMaster.h"
#include "start.h"

namespace Nirvana {
namespace Core {
namespace Windows {

inline
int gtest_sys (int argc, char* argv [], char* envp []) NIRVANA_NOEXCEPT
{
	try {
		Test::GTestSys startup (argc, argv, envp);
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
