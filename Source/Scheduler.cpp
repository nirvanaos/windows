#include "../Port/Scheduler.h"
#include "SchedulerWindows.h"
#include "SchedulerClient.h"

namespace Nirvana {
namespace Core {
namespace Port {

Windows::SchedulerAbstract* Scheduler::scheduler_ = nullptr;

void Scheduler::run (Runnable_ptr startup, DeadlineTime deadline)
{
	Windows::SchedulerWindows impl;
	scheduler_ = &impl;
	impl.run (startup, deadline);
#ifdef _DEBUG
	scheduler_ = nullptr;
#endif
}

void Scheduler::run_client (uint64_t protection_domain, Runnable_ptr startup, DeadlineTime deadline)
{
	Windows::SchedulerClient impl (protection_domain);
	scheduler_ = &impl;
	impl.run (startup, deadline);
#ifdef _DEBUG
	scheduler_ = nullptr;
#endif
}

}
}
}
