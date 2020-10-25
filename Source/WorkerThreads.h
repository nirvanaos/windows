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
	~WorkerThreads ();
	WorkerThreads ();

	void run (Runnable& startup, DeadlineTime deadline);
	void shutdown ();

private:
	struct MainFiberParam
	{
		Core_var <ExecDomain> main_domain;
		ThreadPoolable* worker_thread;
		Runnable* startup;
		DeadlineTime deadline;
	};

	static void CALLBACK main_fiber_proc (void* param);
};

}
}
}

#endif
