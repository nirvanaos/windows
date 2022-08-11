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
#include <limits>

namespace Nirvana {
namespace Core {
namespace Port {

uint64_t Chrono::performance_frequency_;

#ifndef NIRVANA_FAST_MULDIV64
uint64_t Chrono::max_timeout64_;
#endif

void Chrono::initialize () NIRVANA_NOEXCEPT
{
	LARGE_INTEGER pf;
	QueryPerformanceFrequency (&pf);
	performance_frequency_ = pf.QuadPart;
#ifndef NIRVANA_FAST_MULDIV64
	max_timeout64_ = std::numeric_limits <uint64_t>::max () / performance_frequency_;
#endif
}

TimeBase::TimeT Chrono::UTC () NIRVANA_NOEXCEPT
{
	FILETIME ft;
	GetSystemTimePreciseAsFileTime (&ft);
	ULARGE_INTEGER ui;
	ui.LowPart = ft.dwLowDateTime;
	ui.HighPart = ft.dwHighDateTime;
	return ui.QuadPart + WIN_TIME_OFFSET_SEC * 10000000UI64;
}

TimeBase::UtcT Chrono::system_clock () NIRVANA_NOEXCEPT
{
	TimeBase::UtcT t;
	t.time (UTC ());

	TIME_ZONE_INFORMATION tzi;
	GetTimeZoneInformation (&tzi);
	t.tdf ((int16_t)tzi.Bias);

	return t;
}

SteadyTime Chrono::steady_clock () NIRVANA_NOEXCEPT
{
	LARGE_INTEGER pc;
	QueryPerformanceCounter (&pc);
	return pc.QuadPart;
}

DeadlineTime Chrono::make_deadline (TimeBase::TimeT timeout) NIRVANA_NOEXCEPT
{
	uint64_t steady_timeout = 
#ifndef NIRVANA_FAST_MULDIV64
	(timeout <= max_timeout64_) ? timeout * steady_clock_frequency () / 10000000UI64 :
#endif
	muldiv64 (timeout, steady_clock_frequency (), 10000000);
	return steady_clock () + steady_timeout;
}

}
}
}
