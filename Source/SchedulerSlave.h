/// \file
/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERSLAVE_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERSLAVE_H_

#include "SchedulerBase.h"
#include "WorkerThreads.h"
#include "WorkerSemaphore.h"
#include "MessageBroker.h"
#include "Mailslot.h"
#include <PriorityQueue.h>
#include <SkipListWithPool.h>
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
		terminate ();
	}

	/// Main loop.
	/// \returns `false` if system domain is not running.
	bool run (Runnable& startup, DeadlineTime startup_deadline);

	// Implementation of SchedulerAbstract.
	virtual void create_item ();
	virtual void delete_item () NIRVANA_NOEXCEPT;
	virtual void schedule (DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT;
	virtual bool reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) NIRVANA_NOEXCEPT;
	virtual void shutdown () NIRVANA_NOEXCEPT;
	virtual void worker_thread_proc () NIRVANA_NOEXCEPT;

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
