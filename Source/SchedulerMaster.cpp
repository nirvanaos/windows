// Nirvana project
// Windows implementation.
// SchedulerMaster class.

#include "SchedulerMaster.h"
#include "../Port/Scheduler.h"
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

void SchedulerMaster::on_error (int err) NIRVANA_NOEXCEPT
{
	int zero = 0;
	if (error_.compare_exchange_strong (zero, err))
		abort (); // TODO: Send SystemError message to all domains.
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
	try {
		Base::schedule (deadline, executor);
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
	}
}

bool SchedulerMaster::reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) NIRVANA_NOEXCEPT
{
	if (error_)
		return false;

	try {
		if (!Base::reschedule (deadline, executor, old))
			return false;
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
		return false;
	}
	return true;
}

void SchedulerMaster::shutdown () NIRVANA_NOEXCEPT
{
	worker_threads_.shutdown ();
}

void SchedulerMaster::worker_thread_proc () NIRVANA_NOEXCEPT
{
	worker_threads_.thread_proc ();
}

void SchedulerMaster::WorkerThreads::received (OVERLAPPED* ovl, DWORD size) NIRVANA_NOEXCEPT
{
	Executor* executor = reinterpret_cast <Executor*> (ovl);
	ThreadWorker::execute (*executor, 0);
	SchedulerMaster::singleton ().core_free ();
}

}
}
}
