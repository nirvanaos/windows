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
	static void schedule (DeadlineTime deadline, Executor& executor, DeadlineTime old, bool nothrow_fallback)
	{
		scheduler_->schedule (deadline, executor, old, nothrow_fallback);
	}

	static void core_free ()
	{
		scheduler_->core_free ();
	}

	static void shutdown ()
	{
		scheduler_->shutdown ();
	}

	static void run_system_domain (Runnable& startup, DeadlineTime deadline);
	static void run_protection_domain (uint64_t protection_domain, Runnable& startup, DeadlineTime deadline);

private:
	static Windows::SchedulerAbstract* scheduler_;
};

}
}
}

#endif
