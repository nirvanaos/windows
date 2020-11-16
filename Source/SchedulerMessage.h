// Nirvana project
// Windows implementation.
// Scheduler IPC.

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERMESSAGE_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERMESSAGE_H_

#define SCHEDULER_MAILSLOT_NAME MAILSLOT_PREFIX L"scheduler"

#include <core.h>

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
