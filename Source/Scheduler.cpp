#include "../Port/Scheduler.h"
#include "SchedulerMaster.h"
#include "SchedulerSlave.h"

namespace Nirvana {
namespace Core {
namespace Port {

Windows::SchedulerAbstract* Scheduler::scheduler_ = nullptr;

void Scheduler::run_system_domain (Runnable& startup, DeadlineTime deadline)
{
	Windows::SchedulerMaster impl;
	scheduler_ = &impl;
	impl.run (startup, deadline);
#ifdef _DEBUG
	scheduler_ = nullptr;
#endif
}

void Scheduler::run_protection_domain (uint64_t protection_domain, Runnable& startup, DeadlineTime deadline)
{
	Windows::SchedulerSlave impl (protection_domain);
	scheduler_ = &impl;
	impl.run (startup, deadline);
#ifdef _DEBUG
	scheduler_ = nullptr;
#endif
}

}
}
}
