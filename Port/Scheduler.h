#ifndef NIRVANA_CORE_PORT_SCHEDULER_H_
#define NIRVANA_CORE_PORT_SCHEDULER_H_

#include <Scheduler_c.h>
#include <Nirvana/Runnable_c.h>

namespace Nirvana {
namespace Core {
namespace Port {

class Scheduler
{
public:
	
	static void run (Runnable_ptr startup);
	
	static Scheduler_ptr singleton ();
	static void shutdown ();

private:

};

}
}
}

#endif
