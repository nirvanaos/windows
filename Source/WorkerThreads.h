// Nirvana project.
// Windows implementation.
// Worker threads.

#ifndef NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_
#define NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_

#include "ThreadWorker.h"
#include "ThreadPool.h"
#include <ExecDomain.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class WorkerThreads :
	public ThreadPool <ThreadWorker>
{
public:
	WorkerThreads ()
	{
		CompletionPort::start ();
	}

	void run (Runnable_ptr startup, DeadlineTime deadline);
	void shutdown ();

private:
	struct MainFiberParam
	{
		ExecDomain* main_context;
		ThreadPoolable* worker_thread;
		Runnable_ptr startup;
		DeadlineTime deadline;
	};

	static void CALLBACK main_fiber_proc (void* param);
};

}
}
}

#endif
