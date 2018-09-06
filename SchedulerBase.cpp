// Nirvana project
// Windows implementation.
// Scheduler

#include "Scheduler.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerBase::close ()
{
	if (free_cores_semaphore_)
		CloseHandle (free_cores_semaphore_);
	if (INVALID_HANDLE_VALUE != scheduler_mailslot_)
		CloseHandle (scheduler_mailslot_);
}

inline void Scheduler::run_worker_thread ()
{
	if (!deadlines_.empty ()) {
		DeadlineInfo earlier = deadlines_.begin ()->second;
		deadlines_.erase (deadlines_.begin ());
		earlier.process.run (earlier.sync_domain);
	}
}

inline void Scheduler::run ()
{
	OVERLAPPED ovl;
	memset (&ovl, 0, sizeof (ovl));
	ovl.hEvent = (HANDLE)this;

	verify (ReadFileEx (scheduler_mailslot_, &message_, sizeof (message_), &ovl, read_complete));
	do {
		if (deadlines_.empty ())
			SleepEx (INFINITE, TRUE);
		else if (WAIT_OBJECT_0 == WaitForSingleObjectEx (free_cores_semaphore_, INFINITE, TRUE)) {
			verify (ReadFileEx (scheduler_mailslot_, &message_, sizeof (message_), &ovl, read_complete));
			run_worker_thread ();
		}
	} while (!stop_);
}

inline void Scheduler::receive_message (DWORD size)
{
	switch (size) {
	case sizeof (MsgSyncDomainReady):
		{
			Processes::iterator process = processes_.find (message_.sync_domain_ready.process_id);
			assert (process != processes_.end ());
			if (process != processes_.end ())
				process->second.schedule (deadlines_, message_.sync_domain_ready);
		}
		break;

	case sizeof (MsgProcessStart):
		processes_.emplace (message_.process_start.process_id, message_.process_start.process_id);
		break;

	case sizeof (MsgProcessStop):
		{
			Processes::iterator process = processes_.find (message_.process_stop.process_id);
			assert (process != processes_.end ());
			if (process != processes_.end ()) {
				process->second.clear (deadlines_);
				processes_.erase (process);
			}
		}
		break;

	default:
		assert (false);
	}
}

DWORD WINAPI Scheduler::thread_proc (void* _this)
{
	((Scheduler*)_this)->run ();
	return 0;
}

void CALLBACK Scheduler::read_complete (DWORD error_code, DWORD size, OVERLAPPED* ovl)
{
	assert (!error_code);
	((Scheduler*)(ovl->hEvent))->receive_message (size);
}

void CALLBACK Scheduler::stop_proc (ULONG_PTR _this)
{
	((Scheduler*)_this)->stop_ = true;
}

}
}
}
