// Nirvana project.
// Windows implementation.
// Worker threads.

#ifndef NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_
#define NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_

#include "ThreadWorker.h"
#include "ThreadPool.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class WorkerThreads :
	public ThreadPool <ThreadWorker>
{
public:
	WorkerThreads ()
	{
		start (WORKER_THREAD_PRIORITY);
	}

	~WorkerThreads ()
	{}
};

}
}
}

#endif
