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
#include <Nirvana/TimeBase.h>
#include <Nirvana/muldiv64.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// Time API Port implementation.
class Chrono
{
public:
	/// Called on system startup.
	static void initialize () NIRVANA_NOEXCEPT;

	/// Current system time.
	/// NOTE: For the time zone implementation see https://howardhinnant.github.io/date/tz.html
	/// 
	static TimeBase::UtcT system_clock () NIRVANA_NOEXCEPT;

	/// Current UTC time.
	static TimeBase::TimeT UTC () NIRVANA_NOEXCEPT;

	/// Duration since system startup.
	static SteadyTime steady_clock () NIRVANA_NOEXCEPT;

	/// Steady clock frequency, counts per second.
	static const SteadyTime& steady_clock_frequency () NIRVANA_NOEXCEPT
	{
		return performance_frequency_;
	}

	/// Convert UTC time to the local steady time.
	/// 
	/// \param utc UTC time.
	/// \returns Local steady time.
	static SteadyTime UTC_to_steady (TimeBase::TimeT utc) NIRVANA_NOEXCEPT
	{
		return steady_clock () + muldiv64 (utc - UTC (), steady_clock_frequency (), 10000000);
	}

	/// Convert local steady time to UTC time.
	/// 
	/// \param steady Local steady time.
	/// \returns UTC time.
	static TimeBase::TimeT steady_to_UTC (SteadyTime steady) NIRVANA_NOEXCEPT
	{
		return UTC () + muldiv64 (steady - steady_clock (), 10000000, steady_clock_frequency ());
	}

	/// Make deadline.
	/// 
	/// NOTE: If steady_clock_frequency () is too low (1 sec?), Port library can implement advanced
	/// algorithm to create diffirent deadlines inside one clock tick, based on atomic counter.
	///
	/// \param timeout A timeout from the current time.
	/// \return Deadline time as local steady clock value.
	static DeadlineTime make_deadline (TimeBase::TimeT timeout) NIRVANA_NOEXCEPT
	{
		return steady_clock () + muldiv64 (timeout, steady_clock_frequency (), 10000000);
	}

private:
	// Performance counter frequency
	static uint64_t performance_frequency_;

	// Offset from 15 October 1582 00:00:00 to 1 January 1601 12:00:00 in seconds
	static const uint64_t WIN_TIME_OFFSET_SEC = 574862400UI64;
};

}
}
}

#endif
