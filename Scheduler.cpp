// Nirvana project
// Windows implementation.
// Scheduler

#include "Scheduler.h"

namespace Nirvana {
namespace Core {
namespace Windows {

using namespace std;
using namespace CORBA;

WCHAR SchedulerData::scheduler_mailslot_name_ [] = L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\scheduler_mailslot";

HANDLE SchedulerData::open_run_mailslot (DWORD process_id, bool create)
{
	static const WCHAR fmt [] = L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\run_mailslot%08X";
	WCHAR name [_countof (fmt) + 8 - 3];
	wsprintfW (name, fmt, process_id);

	HANDLE mailslot;
	if (create)
		mailslot = CreateMailslotW (name, sizeof (MessageRun), MAILSLOT_WAIT_FOREVER, nullptr);
	else
		mailslot = CreateFileW (name, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE == mailslot)
		throw ::CORBA::INTERNAL ();
	return mailslot;
}

void Scheduler::run_worker_thread ()
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

	ReadFileEx (scheduler_mailslot_, &message_, sizeof (message_), &ovl, read_complete);
	do {
		if (deadlines_.empty ())
			SleepEx (INFINITE, TRUE);
		else if (WAIT_OBJECT_0 == WaitForSingleObjectEx (free_cores_semaphore_, INFINITE, TRUE)) {
			ReadFileEx (scheduler_mailslot_, &message_, sizeof (message_), &ovl, read_complete);
			run_worker_thread ();
		}
	} while (!stop_);
}

void Scheduler::receive_message ()
{
	switch (message_.tag) {
	case MessageSchedule::SYNC_DOMAIN_READY:
		{
			Processes::iterator process = processes_.find (message_.body.sync_domain_ready.process_id);
			assert (process != processes_.end ());
			if (process != processes_.end ())
				process->second.schedule (deadlines_, message_.body.sync_domain_ready);
		}
		break;

	case MessageSchedule::PROCESS_START:
		processes_.emplace (message_.body.process_start.process_id, message_.body.process_start.process_id);
		break;

	case MessageSchedule::PROCESS_STOP:
		{
			Processes::iterator process = processes_.find (message_.body.process_stop.process_id);
			assert (process != processes_.end ());
			if (process != processes_.end ()) {
				process->second.clear (deadlines_);
				processes_.erase (process);
			}
		}
		break;
	}
}

DWORD WINAPI Scheduler::thread_proc (void* _this)
{
	((Scheduler*)_this)->run ();
	return 0;
}

void CALLBACK Scheduler::read_complete (DWORD error_code, DWORD size, OVERLAPPED* ovl)
{
	((Scheduler*)(ovl->hEvent))->receive_message ();
}

void CALLBACK Scheduler::stop_proc (ULONG_PTR _this)
{
	((Scheduler*)_this)->stop_ = true;
}

}
}
}
