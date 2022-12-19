/// \file
/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
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
#pragma once

#include "SchedulerBase.h"
#include "WorkerThreads.h"
#include "WorkerSemaphore.h"
#include "Mailslot.h"
#include <PriorityQueue.h>
#include <SkipListWithPool.h>
#include <atomic>

namespace Nirvana {
namespace Core {

class StartupProt;

namespace Windows {

class SchedulerSlave :
	public SchedulerBase
{
public:
	SchedulerSlave ();

	~SchedulerSlave ()
	{
		terminate ();
	}

	/// Main loop.
	/// 
	/// \param startup Protection domain startup runnable.
	/// \param startup_deadline Protection domain startup deadline.
	/// \returns `false` if system domain is not running.
	bool run (StartupProt& startup, DeadlineTime startup_deadline);

	// Implementation of SchedulerAbstract.
	virtual void create_item ();
	virtual void delete_item () NIRVANA_NOEXCEPT;
	virtual void schedule (DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT;
	virtual bool reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) NIRVANA_NOEXCEPT;
	virtual void shutdown () NIRVANA_NOEXCEPT;

	// Implementation of SchedulerBase
	virtual void worker_thread_proc () NIRVANA_NOEXCEPT;

	// Called from worker thread.
	void execute () NIRVANA_NOEXCEPT;

private:
	void terminate ();
	void core_free () NIRVANA_NOEXCEPT;
	void on_error (int err) NIRVANA_NOEXCEPT;

	static DWORD WINAPI s_watchdog_thread_proc (void* _this);

private:
	HANDLE sys_process_;
	HANDLE terminate_event_;
	HANDLE watchdog_thread_;
	Mailslot scheduler_mailslot_;
	uint32_t executor_id_;
	std::atomic <int> error_;
	SkipListWithPool <PriorityQueue <Executor*, PROT_DOMAIN_PRIORITY_QUEUE_LEVELS> > queue_;
	WorkerThreads <WorkerSemaphore> worker_threads_;
};

}
}
}

#endif
