#include "../Port/Scheduler.h"
#include "SchedulerMaster.h"
#include "SchedulerSlave.h"

namespace Nirvana {
namespace Core {
namespace Port {

Windows::SchedulerAbstract* Scheduler::singleton_ = nullptr;

bool Scheduler::run_sys_domain (Runnable& startup, DeadlineTime deadline)
{
	return Windows::SchedulerMaster ().run (startup, deadline);
}

bool Scheduler::run_prot_domain (Runnable& startup, DeadlineTime deadline)
{
	return Windows::SchedulerSlave ().run (startup, deadline);
}

}
}
}
