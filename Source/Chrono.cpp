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

uint64_t Chrono::TSC_frequency_;

#ifndef NIRVANA_FAST_RESCALE64
uint64_t Chrono::max_timeout64_;
#endif

void Chrono::initialize () NIRVANA_NOEXCEPT
{
	TSC_frequency_ = 0;
	{
		int info [4];
		__cpuid (info, 0);
		int maxfunc = info [0];
		if (maxfunc >= 0x15) {
			__cpuid (info, 0x15);
			if (info [1] && info [2])
				TSC_frequency_ = UInt32x32To64 (info [2], info [1]) / info [0];
		}
	}

	if (!TSC_frequency_) {
		LARGE_INTEGER pf;
		QueryPerformanceFrequency (&pf);

		LARGE_INTEGER pc_start;
		int prio = GetThreadPriority (GetCurrentThread ());
		verify (SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL));
		QueryPerformanceCounter (&pc_start);
		uint64_t start = __rdtsc ();
		verify (SetThreadPriority (GetCurrentThread (), prio));
		
		Sleep (100);
		
		LARGE_INTEGER pc_end;
		verify (SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL));
		QueryPerformanceCounter (&pc_end);
		uint64_t end = __rdtsc ();
		verify (SetThreadPriority (GetCurrentThread (), prio));
		
		TSC_frequency_ = rescale64 (end - start, pf.QuadPart, 0, pc_end.QuadPart - pc_start.QuadPart);
	}

#ifndef NIRVANA_FAST_RESCALE64
	max_timeout64_ = std::numeric_limits <uint64_t>::max () / TSC_frequency_;
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

DeadlineTime Chrono::make_deadline (TimeBase::TimeT timeout) NIRVANA_NOEXCEPT
{
	uint64_t dt_timeout = 
#ifndef NIRVANA_FAST_MULDIV64
	(timeout <= max_timeout64_) ? ((timeout * deadline_clock_frequency () + 9999999) / 10000000) :
#endif
	rescale64 ((int64_t)timeout, deadline_clock_frequency (), 9999999I64, 10000000UI64);
	return steady_clock () + dt_timeout;
}

}
}
}
