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

#define SCHEDULER_MAILSLOT_NAME MAILSLOT_PREFIX L"scheduler"

#include <Nirvana/NirvanaBase.h>

namespace Nirvana {
namespace Core {
namespace Windows {

#pragma pack (push, 1)

struct SchedulerMessage
{
	struct Schedule
	{
		DeadlineTime deadline;
		uint32_t executor_id;

		Schedule (const DeadlineTime& dt, uint32_t id) :
			deadline (dt),
			executor_id (id)
		{}
	};

	struct ReSchedule
	{
		DeadlineTime deadline;
		DeadlineTime deadline_prev;
		uint32_t executor_id;

		ReSchedule (const DeadlineTime& dt, uint32_t id, const DeadlineTime& old) :
			deadline (dt),
			deadline_prev (old),
			executor_id (id)
		{}
	};

	struct Tagged
	{
		enum Tag : uint32_t
		{
			CREATE_ITEM,
			DELETE_ITEM,
			CORE_FREE
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
