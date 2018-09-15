// Nirvana project
// Windows implementation.
// SchedulerWindows class.

#include "SchedulerWindows.h"
#include "../Thread.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerWindows::_schedule (::CORBA::Nirvana::Bridge <Scheduler>* bridge,
																	DeadlineTime deadline, ::CORBA::Nirvana::Bridge <Runnable>* runnable,
																	DeadlineTime deadline_prev, ::CORBA::Nirvana::EnvironmentBridge*)
{
	SchedulerItem item = {nullptr, (uint64_t)runnable};
	static_cast <Base*> (static_cast <SchedulerWindows*> (bridge))->schedule (deadline, item, deadline_prev);
}

void SchedulerWindows::_core_free (::CORBA::Nirvana::Bridge <Scheduler>* bridge, ::CORBA::Nirvana::EnvironmentBridge*)
{
	static_cast <Base*> (static_cast <SchedulerWindows*> (bridge))->core_free ();
}

void SchedulerWindows::InProcExecute::received (OVERLAPPED* ovl, DWORD size)
{
	Thread::execute (reinterpret_cast <::CORBA::Nirvana::Bridge <Runnable>*> (ovl));
}

}
}
}
