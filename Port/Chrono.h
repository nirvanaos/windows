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
#ifndef NIRVANA_CORE_PORT_CHRONO_H_
#define NIRVANA_CORE_PORT_CHRONO_H_

#include <stdint.h>

namespace Nirvana {
namespace Core {
namespace Port {

class Chrono
{
public:
	static void initialize ();

	static const uint16_t epoch = 2021;

	static uint64_t system_clock ();
	static uint64_t steady_clock ();

	/// Returns steady clock resolution in ns
	static uint32_t steady_clock_resoluion ()
	{
		return time_increment_;
	}

private:
	static unsigned long time_increment_; // Time between system ticks in nanoseconds

	static const uint64_t WIN_TIME_OFFSET_SEC = 13253068800UI64;
};

}
}
}

#endif
