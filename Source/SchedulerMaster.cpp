// Nirvana project
// Windows implementation.
// SchedulerMaster class.

#include "SchedulerMaster.h"
#include "../Port/Scheduler.h"
#include <Thread.h>
#include "MailslotName.h"

namespace Nirvana {
namespace Core {
namespace Windows {

bool SchedulerMaster::run (Runnable& startup, DeadlineTime deadline)
{
	if (!(
		Office::create_mailslot (SCHEDULER_MAILSLOT_NAME)
		&&
		message_broker_.create_mailslot (MailslotName (0))
		))
		return false;
	Office::start ();
	message_broker_.start ();
	WorkerThreads::run (startup, deadline);
}

void SchedulerMaster::schedule (DeadlineTime deadline, Executor& executor, DeadlineTime deadline_prev, bool nothrow_fallback)
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

void SchedulerMaster::core_free ()
{
	Base::core_free ();
}

void SchedulerMaster::InProcExecute::received (OVERLAPPED* ovl, DWORD size)
{
	Execute* exec = reinterpret_cast <Execute*> (ovl);
	ThreadWorker::execute (*reinterpret_cast <Executor*> (exec->executor), (Word)exec->scheduler_error);
	scheduler ().core_free ();
}

}
}
}
