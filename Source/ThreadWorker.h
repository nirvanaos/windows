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
#ifndef NIRVANA_CORE_WINDOWS_THREADWORKER_H_
#define NIRVANA_CORE_WINDOWS_THREADWORKER_H_
#pragma once

#include <ThreadWorker.h>
#include "Thread.inl"

namespace Nirvana {
namespace Core {

class Startup;

namespace Windows {

class WorkerSemaphore;
class CompletionPort;

/// Platform-specific worker thread implementation.
class ThreadWorker final :
	public Core::ThreadWorker
{
public:
	void create ()
	{
		port ().create (this, WORKER_THREAD_PRIORITY);
	}

	void run_main (Startup& startup, DeadlineTime deadline, ThreadWorker* other_workers, size_t other_worker_cnt);

	ThreadWorker (Windows::WorkerSemaphore& master)
	{}

	ThreadWorker (Windows::CompletionPort& master)
	{}

	~ThreadWorker ()
	{}

	void execute (Ref <Executor>&& executor) noexcept
	{
		static_cast <Core::ThreadWorker&> (*this).execute (std::move (executor));
		RevertToSelf ();
	}

	void join ()
	{
		Port::Thread::join ();
	}

private:
	friend class Port::Thread;
	static unsigned long __stdcall thread_proc (ThreadWorker* _this);
	struct MainNeutralFiberParam;
	static void __stdcall main_neutral_fiber_proc (MainNeutralFiberParam* param);
};

}
}
}

#endif
