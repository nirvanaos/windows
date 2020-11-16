// Nirvana project.
// Windows implementation.
// Worker threads.

#ifndef NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_
#define NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_

#include "TaskMaster.h"
#include <ThreadWorker.h>
#include "ThreadWorker.inl"
#include "ThreadPool.h"

namespace Nirvana {
namespace Core {
namespace Windows {

template <class Master>
class WorkerThreads :
	public ThreadPool <Master, Core::ThreadWorker>
{
	typedef Core::ThreadWorker ThreadType;
	typedef ThreadPool <TaskMaster, ThreadType> Base;
public:
	void run (Runnable& startup, DeadlineTime deadline);
};

}
}
}

#endif
