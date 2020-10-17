#include <Heap.h>
#include <Scheduler.h>
#include <Runnable.h>
#include <limits>

namespace Nirvana {
namespace Core {

class Startup :
	public ImplStatic <Runnable>
{
public:
	void run ()
	{
		::Nirvana::Core::Scheduler::shutdown ();
	}
};

}
}

int main ()
{
	Nirvana::Core::Startup runnable;
	Nirvana::Core::Port::Scheduler::run_system_domain (runnable, std::numeric_limits <Nirvana::DeadlineTime>::max ());
	return 0;
}
