// Nirvana project
// Windows implementation.
// SchedulerWindows class.

#include "SchedulerWindows.h"
#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerWindows::_schedule (::CORBA::Nirvana::Bridge <Scheduler>* bridge,
																	DeadlineTime deadline, ::CORBA::Nirvana::BridgeMarshal <Executor>* executor,
																	DeadlineTime deadline_prev, ::CORBA::Nirvana::EnvironmentBridge*)
{
	static_cast <Base*> (static_cast <SchedulerWindows*> (bridge))->schedule (deadline, SchedulerItem (executor), deadline_prev);
}

void SchedulerWindows::_core_free (::CORBA::Nirvana::Bridge <Scheduler>* bridge, ::CORBA::Nirvana::EnvironmentBridge*)
{
	static_cast <Base*> (static_cast <SchedulerWindows*> (bridge))->core_free ();
}

void SchedulerWindows::InProcExecute::received (OVERLAPPED* ovl, DWORD size)
{
	Execute* exec = reinterpret_cast <Execute*> (ovl);
	Thread::execute (reinterpret_cast < ::CORBA::Nirvana::Bridge <Executor>*> (exec->executor), exec->deadline);
}

}
}
}
