#include "../Port/Scheduler.h"
#include "SchedulerMaster.h"
#include "SchedulerSlave.h"
#include <iostream>

namespace Nirvana {
namespace Core {
namespace Port {

using namespace std;

Windows::SchedulerAbstract* Scheduler::singleton_ = nullptr;

void Scheduler::run_sys_domain (Runnable& startup, DeadlineTime deadline) NIRVANA_NOEXCEPT
{
	if (!Windows::SchedulerMaster ().run (startup, deadline))
		cout << "System is already running." << endl;
}

void Scheduler::run_prot_domain (Runnable& startup, DeadlineTime deadline) NIRVANA_NOEXCEPT
{
	if (!Windows::SchedulerSlave ().run (startup, deadline))
		cout << "System is not running." << endl;
}

}
}
}
