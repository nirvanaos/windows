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
	worker_threads_.run (startup, deadline);
	return true;
}

void SchedulerMaster::create_item ()
{
	Base::create_item ();
}

void SchedulerMaster::delete_item () NIRVANA_NOEXCEPT
{
	Base::delete_item ();
}

void SchedulerMaster::schedule (DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT
{
	Base::schedule (deadline, executor);
}

void SchedulerMaster::WorkerThreads::received (OVERLAPPED* ovl, DWORD size)
{
	Executor* executor = reinterpret_cast <Executor*> (ovl);
	ThreadWorker::execute (*executor, 0);
	SchedulerMaster::singleton ().core_free ();
}

}
}
}
