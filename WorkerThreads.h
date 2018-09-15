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
	static void initialize ()
	{
		assert (!singleton_);
		singleton_ = new WorkerThreads ();
	}

	static void terminate ()
	{
		delete singleton_;
		singleton_ = nullptr;
	}

	static WorkerThreads& singleton ()
	{
		return *singleton_;
	}

private:
	WorkerThreads ()
	{
		start (WORKER_THREAD_PRIORITY);
	}

	~WorkerThreads ()
	{}

private:
	static WorkerThreads* singleton_;
};

}
}
}

#endif
