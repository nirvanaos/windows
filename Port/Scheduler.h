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
#ifndef NIRVANA_CORE_PORT_SCHEDULER_H_
#define NIRVANA_CORE_PORT_SCHEDULER_H_
#pragma once

#include "../Source/SchedulerAbstract.h"

// Output debug messages on shutdown.
#define DEBUG_SHUTDOWN

namespace Nirvana {
namespace Core {
namespace Port {

class Scheduler
{
	///@{
	/// Members called from Core.
public:
	/// Reserve space for an active item.
	/// \throws CORBA::NO_MEMORY
	static void create_item (bool with_reschedule)
	{
		singleton_->create_item (with_reschedule);
	}

	/// Release active item space.
	static void delete_item (bool with_reschedule) noexcept
	{
		if (singleton_)
			singleton_->delete_item (with_reschedule);
	}

	/// \summary Schedule execution.
	/// 
	/// \param deadline Deadline.
	/// \param executor Executor.
	static void schedule (const DeadlineTime& deadline, Executor& executor) noexcept
	{
		singleton_->schedule (deadline, executor);
	}

	/// \summary Re-schedule execution.
	/// 
	/// \param deadline New deadline.
	/// \param executor Executor.
	/// \param old Old deadline.
	/// \returns `true` if the executor was found and rescheduled. `false` if executor with old deadline was not found.
	static bool reschedule (const DeadlineTime& deadline, Executor& executor, const DeadlineTime& old) noexcept
	{
		return singleton_->reschedule (deadline, executor, old);
	}

	/// Initiate shutdown for the current domain.
	static void shutdown () noexcept
	{
		singleton_->shutdown ();
	}

	///@}

protected:
	static Windows::SchedulerAbstract* singleton_;
};

}
}
}

#endif
