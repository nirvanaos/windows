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
#pragma once

#include <CORBA/CORBA.h>
#include <CORBA/TimeBase.h>
#include <Nirvana/time_defs.h>
#include <Nirvana/rescale.h>

extern "C" __declspec(dllimport)
void __stdcall QueryInterruptTimePrecise (unsigned __int64* lpInterruptTimePrecise);

namespace Nirvana {
namespace Core {
namespace Port {

/// Time API Port implementation.
class Chrono
{
public:
	/// Current system time.
	/// NOTE: For the time zone implementation see https://howardhinnant.github.io/date/tz.html
	/// 
	static TimeBase::UtcT system_clock () NIRVANA_NOEXCEPT;

	/// Current UTC time.
	static TimeBase::UtcT UTC () NIRVANA_NOEXCEPT;

	/// Duration since system startup in 100 ns intervals.
	static SteadyTime steady_clock () NIRVANA_NOEXCEPT
	{
		unsigned __int64 t;
		QueryInterruptTimePrecise (&t);
		return t;
	}

	/// Duration since system startup.
	static DeadlineTime deadline_clock () NIRVANA_NOEXCEPT
	{
		return __rdtsc ();
	}

	/// Deadline clock frequency, Hz.
	static const DeadlineTime& deadline_clock_frequency () NIRVANA_NOEXCEPT
	{
		return TSC_frequency_;
	}

	/// Convert UTC time to the local deadline time.
	/// 
	/// \param utc UTC time.
	/// \returns Local deadline time.
	static DeadlineTime deadline_from_UTC (TimeBase::UtcT utc) NIRVANA_NOEXCEPT
	{
		TimeBase::UtcT cur = UTC ();
		return deadline_clock () + rescale64 (utc.time () - cur.time () + inacc_max (utc, cur),
			deadline_clock_frequency (), 0, 10000000);
	}

	/// Convert local deadline time to UTC time.
	/// 
	/// \param deadline Local deadline time.
	/// \returns UTC time.
	static TimeBase::UtcT deadline_to_UTC (DeadlineTime deadline) NIRVANA_NOEXCEPT
	{
		TimeBase::UtcT utc = UTC ();
		utc.time (utc.time () + rescale64 (deadline - deadline_clock (), 10000000,
			deadline_clock_frequency () - 1, deadline_clock_frequency ()));
		return utc;
	}

	/// Make deadline.
	/// 
	/// NOTE: If deadline_clock_frequency () is too low (1 sec?), Port library can implement advanced
	/// algorithm to create diffirent deadlines inside one clock tick, based on atomic counter.
	///
	/// \param timeout A timeout from the current time.
	/// \return Deadline time as local steady clock value.
	static DeadlineTime make_deadline (TimeBase::TimeT timeout) NIRVANA_NOEXCEPT;

	static void initialize () NIRVANA_NOEXCEPT;
	static void terminate () NIRVANA_NOEXCEPT;

private:
	static uint64_t inacc_max (const TimeBase::UtcT& t1, const TimeBase::UtcT& t2) NIRVANA_NOEXCEPT
	{
		if (t1.inacchi () > t2.inacchi ())
			return ((uint64_t)t1.inacchi () << 32) | t1.inacclo ();
		else if (t1.inacchi () < t2.inacchi ())
			return ((uint64_t)t2.inacchi () << 32) | t2.inacclo ();
		else
			return ((uint64_t)t1.inacchi () << 32) | std::max (t1.inacclo (), t2.inacclo ());
	}

	static bool NTP_client_enabled ();
	static uint32_t local_clock_dispersion_sec ();
	static uint32_t max_allowed_phase_offset_sec ();
	static bool adjustment_in_progress ();

private:
	// Performance counter frequency
	static uint64_t TSC_frequency_;

#ifndef NIRVANA_FAST_RESCALE64
	// Maximal timeout fit in 64-bit multiplication
	static uint64_t max_timeout64_;
#endif

	static void* hkey_time_config_;
	static void* hkey_time_client_;
};

}
}
}

#endif
