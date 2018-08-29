// Nirvana project.
// Windows implementation.
// Protection domain (process).

#ifndef NIRVANA_CORE_WINDOWS_PROTDOMAINBASE_H_
#define NIRVANA_CORE_WINDOWS_PROTDOMAINBASE_H_

#include "Scheduler.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class ProtDomainBase : 
	private SchedulerData
{
public:
	void initialize ();
	void terminate ();

	static void schedule (DeadlineTime deadline, void* sync_domain, bool update)
	{
		MessageSchedule msg;
		msg.tag = MessageSchedule::SYNC_DOMAIN_READY;
		msg.body.sync_domain_ready.deadline = deadline;
		msg.body.sync_domain_ready.process_id = GetCurrentProcessId ();
		msg.body.sync_domain_ready.sync_domain = (ULONG_PTR)sync_domain;
		msg.body.sync_domain_ready.update = update;

		DWORD cb;
		if (!WriteFile (scheduler_mailslot_, &msg, sizeof (msg), &cb, nullptr))
			throw ::CORBA::INTERNAL ();
	}

private:
	static DWORD WINAPI thread_proc (void* _this);

private:
	static HANDLE scheduler_mailslot_;
	static HANDLE run_mailslot_;
	static HANDLE free_cores_semaphore_;
};

}
}
}

#endif
