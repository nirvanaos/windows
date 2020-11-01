// Nirvana project
// Windows implementation.
// SchedulerWindows class.

#include "SchedulerWindows.h"
#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerWindows::schedule (DeadlineTime deadline, Executor& executor, DeadlineTime deadline_prev, bool nothrow_fallback)
{
	try {
		Base::schedule (deadline, SchedulerItem (executor), deadline_prev);
	} catch (...) {
		if (nothrow_fallback) {
			// Fallback
			Execute exec;
			exec.executor = (uint64_t)&executor;
			exec.deadline = deadline;
			exec.scheduler_error = CORBA::SystemException::EC_NO_MEMORY;
			in_proc_execute_.execute (exec);
		} else
			throw;
	}
}

void SchedulerWindows::core_free ()
{
	Base::core_free ();
}

void SchedulerWindows::InProcExecute::received (OVERLAPPED* ovl, DWORD size)
{
	Execute* exec = reinterpret_cast <Execute*> (ovl);
	ThreadWorker::execute (*reinterpret_cast <Executor*> (exec->executor), exec->deadline, (Word)exec->scheduler_error);
}

}
}
}
