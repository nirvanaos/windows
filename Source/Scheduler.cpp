#include "../Port/Scheduler.h"
#include "SchedulerWindows.h"
#include "SchedulerClient.h"

namespace Nirvana {
namespace Core {
namespace Port {

Scheduler_ptr Scheduler::scheduler_;
Scheduler::Implementation Scheduler::implementation_;
bool Scheduler::sys_domain_ = false;

void Scheduler::run (Runnable_ptr startup, DeadlineTime deadline)
{
	sys_domain_ = true;
	scheduler_ = implementation_.sys_domain = new Windows::SchedulerWindows ();
	implementation_.sys_domain->run (startup, deadline);
	CORBA::release (scheduler_);
}

void Scheduler::shutdown ()
{
	if (sys_domain_)
		implementation_.sys_domain->shutdown ();
	else
		implementation_.prot_domain->shutdown ();
}

}
}
}
