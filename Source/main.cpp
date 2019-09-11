#include <Heap.h>
#include <Scheduler.h>
#include <Nirvana/Runnable_s.h>
#include <limits>

namespace Nirvana {
namespace Core {

class Startup :
	public ::CORBA::Nirvana::Servant <Startup, Runnable>,
	public ::CORBA::Nirvana::LifeCycleStatic <>
{
public:
	void run ()
	{
		::Nirvana::Core::Scheduler::shutdown ();
	}
};

namespace Windows {

int main ()
{
	Heap::initialize ();
	Startup runnable;
	::Nirvana::Core::Port::Scheduler::run_system_domain (runnable._get_ptr (), std::numeric_limits <DeadlineTime>::max ());
	Heap::terminate ();
	return 0;
}

}
}
}