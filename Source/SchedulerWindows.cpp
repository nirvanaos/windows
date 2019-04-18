// Nirvana project
// Windows implementation.
// SchedulerWindows class.

#include "SchedulerWindows.h"
#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerWindows::schedule (DeadlineTime deadline, Executor& executor, DeadlineTime deadline_prev)
{
	Base::schedule (deadline, SchedulerItem (executor), deadline_prev);
}

void SchedulerWindows::core_free ()
{
	Base::core_free ();
}

void SchedulerWindows::InProcExecute::received (OVERLAPPED* ovl, DWORD size)
{
	Execute* exec = reinterpret_cast <Execute*> (ovl);
	Thread::execute (*reinterpret_cast <Executor*> (exec->executor), exec->deadline);
}

}
}
}
