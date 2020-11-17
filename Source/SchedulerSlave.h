#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERSLAVE_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERSLAVE_H_

#include "SchedulerBase.h"
#include "WorkerThreads.h"
#include "WorkerSemaphore.h"
#include "MessageBroker.h"
#include "Mailslot.h"
#include <PriorityQueue.h>
#include <SkipListWithPool.h>
#include <StartupProt.h>
#include <atomic>

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerSlave :
	public SchedulerBase <SchedulerSlave>
{
public:
	/// Used when process started by the system domain.
	SchedulerSlave (uint32_t sys_process_id, uint32_t sys_semaphore);

	/// Used when process started by user.
	SchedulerSlave ();

	~SchedulerSlave ()
	{
		if (sys_process_)
			CloseHandle (sys_process_);
	}

	/// Main loop.
	/// \param argc Command line argumens count.
	/// \param argv Command line argumens.
	/// \param deadline Startup deadline.
	/// \returns `false` if system domain is not running.
	bool run (int argc, char* argv [], DeadlineTime deadline = StartupProt::default_deadline ());

	// Implementation of SchedulerAbstract.
	virtual void create_item ();
	virtual void delete_item () NIRVANA_NOEXCEPT;
	virtual void schedule (DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT;
	virtual bool reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) NIRVANA_NOEXCEPT;
	virtual void shutdown () NIRVANA_NOEXCEPT;

	// Called from worker thread.
	void execute () NIRVANA_NOEXCEPT;

private:
	bool initialize ();
	void initialize (uint32_t sys_process_id, uint32_t sys_semaphore);
	void terminate ();
	void core_free () NIRVANA_NOEXCEPT;
	void on_error (int err) NIRVANA_NOEXCEPT;

private:
	HANDLE sys_process_;
	Mailslot scheduler_mailslot_;
	Mailslot sys_mailslot_;
	MessageBroker message_broker_;
	uint32_t executor_id_;
	std::atomic <int> error_;
	SkipListWithPool <PriorityQueue <Executor*, PROT_DOMAIN_PRIORITY_QUEUE_LEVELS> > queue_;
	WorkerThreads <WorkerSemaphore> worker_threads_;
};

}
}
}

#endif
