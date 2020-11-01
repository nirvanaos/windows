// Nirvana project.
// Windows implementation.
// Worker threads.

#ifndef NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_
#define NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_

#include "ThreadWorkerInternal.h"
#include "ThreadPool.h"
#include <ExecDomain.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class WorkerThreads :
	public ThreadPool <ImplStatic <Core::ThreadWorker> >
{
	typedef ImplStatic <Core::ThreadWorker> ThreadType;
	typedef ThreadPool <ImplStatic <Core::ThreadWorker> > Base;
public:
	WorkerThreads ()
	{
		CompletionPort::start ();
	}

	~WorkerThreads ()
	{}

	void run (Runnable& startup, DeadlineTime deadline);
	void shutdown ();
};

}
}
}

#endif
