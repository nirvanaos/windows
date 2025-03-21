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
#include <PriorityQueueReorder.h>
#include <SkipListWithPool.h>

namespace Nirvana {
namespace Core {

class StartupProt;

namespace Windows {

class SchedulerSlave :
	public SchedulerBase
{
public:
	SchedulerSlave ();
	~SchedulerSlave ();

	/// Main loop.
	/// 
	/// \param startup Protection domain startup runnable.
	/// \param startup_deadline Protection domain startup deadline.
	/// \returns `false` if system domain is not running.
	bool run (StartupProt& startup, DeadlineTime startup_deadline);

	// Implementation of SchedulerAbstract.
	virtual void create_item ();
	virtual void delete_item () noexcept;
	virtual void schedule (DeadlineTime deadline, Executor& executor) noexcept;
	virtual bool reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) noexcept;
	virtual void shutdown () noexcept;

	// Implementation of SchedulerBase
	virtual void worker_thread_proc () noexcept;

	// Called from worker thread.
	void execute () noexcept;

private:
	void core_free () noexcept;

	template <class MSG>
	void send (const MSG& msg) const
	{
		DWORD cb;
		if (!WriteFile (scheduler_pipe_, &msg, sizeof (MSG), &cb, nullptr))
			throw_COMM_FAILURE ();
	}

	static DWORD WINAPI s_watchdog_thread_proc (void* _this);

private:
	HANDLE sys_process_;
	HANDLE terminate_event_;
	HANDLE watchdog_thread_;
	HANDLE scheduler_pipe_;
	SkipListWithPool <PriorityQueueReorder <Ref <Executor>, SKIP_LIST_DEFAULT_LEVELS> > queue_;
	WorkerThreads <WorkerSemaphore> worker_threads_;
};

}
}
}

#endif
