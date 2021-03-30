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
#ifndef NIRVANA_CORE_WINDOWS_THREADWORKER_H_
#define NIRVANA_CORE_WINDOWS_THREADWORKER_H_

#include <ThreadWorker.h>
#include "Thread.inl"
#include "ExecContext.inl"

namespace Nirvana {
namespace Core {

class Runnable;

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

	void run_main (Runnable& startup, DeadlineTime deadline);

	ThreadWorker (Windows::WorkerSemaphore& master)
	{}

	ThreadWorker (Windows::CompletionPort& master)
	{}

	~ThreadWorker ()
	{}

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
