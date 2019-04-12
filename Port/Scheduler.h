#ifndef NIRVANA_CORE_PORT_SCHEDULER_H_
#define NIRVANA_CORE_PORT_SCHEDULER_H_

#include "../Source/Scheduler_c.h"
#include <Nirvana/Runnable_c.h>

namespace Nirvana {
namespace Core {

namespace Windows {
class SchedulerWindows;
class SchedulerClient;
}

namespace Port {

class Scheduler
{
public:
	
	static void run (Runnable_ptr startup, DeadlineTime deadline);
	
	static Scheduler_ptr singleton ()
	{
		return scheduler_;
	}

	static void shutdown ();

private:
	static Scheduler_ptr scheduler_;

	static union Implementation
	{
		Windows::SchedulerWindows* sys_domain;
		Windows::SchedulerClient* prot_domain;
	} implementation_;
	
	static bool sys_domain_;
};

}
}
}

#endif
