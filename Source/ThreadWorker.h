// Nirvana project
// Windows implementation.
// ThreadWorker class.
// Platform-specific worker thread implementation.

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
