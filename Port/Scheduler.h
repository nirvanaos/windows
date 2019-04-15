#ifndef NIRVANA_CORE_PORT_SCHEDULER_H_
#define NIRVANA_CORE_PORT_SCHEDULER_H_

#include "../Source/Scheduler_c.h"
#include <Nirvana/Runnable_c.h>

namespace Nirvana {
namespace Core {

namespace Windows {
class SchedulerAbstract;
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

	static void run_client (uint64_t protection_domain, Runnable_ptr startup, DeadlineTime deadline);

private:
	static Core::Scheduler_ptr scheduler_;
};

}
}
}

#endif
