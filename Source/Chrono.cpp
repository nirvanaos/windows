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
#include "../Port/Chrono.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

unsigned long Chrono::time_increment_;

void Chrono::initialize () NIRVANA_NOEXCEPT
{
	// TODO: Use KeQueryTimeIncrement ();
	uint64_t t0;
	QueryInterruptTimePrecise (&t0);
	uint64_t inc;
	uint64_t t1;
	for (;;) {
		QueryInterruptTimePrecise (&t1);
		if ((inc = t1 - t0))
			break;
	}
	for (size_t check_cnt = 0; check_cnt < 3;) {
		t0 = t1;
		uint64_t inc1;
		for (;;) {
			QueryInterruptTimePrecise (&t1);
			if ((inc1 = t1 - t0))
				break;
		}
		if (inc1 == inc)
			++check_cnt;
		else {
			if (inc1 < inc)
				inc = inc1;
			check_cnt = 0;
		}
	}

	time_increment_ = (unsigned long)inc * 100;
}

uint64_t Chrono::system_clock () NIRVANA_NOEXCEPT
{
	FILETIME ft;
	GetSystemTimePreciseAsFileTime (&ft);
	ULARGE_INTEGER ui;
	ui.LowPart = ft.dwLowDateTime;
	ui.HighPart = ft.dwHighDateTime;
	return (ui.QuadPart - WIN_TIME_OFFSET_SEC * 10000000UI64) * 100UI64;
}

uint64_t Chrono::steady_clock () NIRVANA_NOEXCEPT
{
	ULONGLONG t;
	QueryInterruptTimePrecise (&t);
	return t * 100UI64;
}

}
}
}
