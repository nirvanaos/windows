#ifndef NIRVANA_CORE_PORT_SCHEDULER_H_
#define NIRVANA_CORE_PORT_SCHEDULER_H_

#include "../Source/SchedulerAbstract.h"
#include <Runnable.h>

namespace Nirvana {
namespace Core {
namespace Port {

class Scheduler
{
public:
	static void create_item ()
	{
		return scheduler_->create_item ();
	}

	static void delete_item () NIRVANA_NOEXCEPT
	{
		scheduler_->delete_item ();
	}

	static void schedule (const DeadlineTime& deadline, Executor& executor) NIRVANA_NOEXCEPT
	{
		scheduler_->schedule (deadline, executor);
	}

	static bool reschedule (const DeadlineTime& deadline, Executor& executor, const DeadlineTime& old) NIRVANA_NOEXCEPT
	{
		return scheduler_->reschedule (deadline, executor, old);
	}

	static void shutdown () NIRVANA_NOEXCEPT
	{
		scheduler_->shutdown ();
	}

	static void run_system_domain (Runnable& startup, DeadlineTime deadline);
	static void run_protection_domain (uint64_t protection_domain, Runnable& startup, DeadlineTime deadline);

protected:
	static Windows::SchedulerAbstract* scheduler_;
};

}
}
}

#endif
