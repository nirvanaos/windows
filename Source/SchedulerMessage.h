/// \file
/// Scheduler IPC.
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
#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERMESSAGE_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERMESSAGE_H_
#pragma once

#include <Nirvana/NirvanaBase.h>

#define SCHEDULER_PIPE_NAME WINWCS ("\\\\.\\pipe\\") OBJ_NAME_PREFIX WINWCS ("/scheduler")
#define SCHEDULER_SEMAPHORE_PREFIX OBJ_NAME_PREFIX WINWCS ("/scheduler")

namespace Nirvana {
namespace Core {
namespace Windows {

#pragma pack (push, 1)

struct SchedulerMessage
{
	struct Schedule
	{
		DeadlineTime deadline;

		Schedule (const DeadlineTime& dt) :
			deadline (dt)
		{}
	};

	struct ReSchedule
	{
		DeadlineTime deadline;
		DeadlineTime deadline_prev;

		ReSchedule (const DeadlineTime& dt, const DeadlineTime& old) :
			deadline (dt),
			deadline_prev (old)
		{}
	};

	struct Tagged
	{
		enum Tag : uint32_t
		{
			CORE_FREE,
			CREATE_ITEM,
			CREATE_ITEM2,
			DELETE_ITEM,
			DELETE_ITEM2
		}
		tag;

		Tagged (Tag t) :
			tag (t)
		{}
	};

	union Buffer
	{
		Schedule schedule;
		ReSchedule reschedule;
		Tagged tagged;
	};
};

#pragma pack (pop)

}
}
}

#endif
