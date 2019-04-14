// Nirvana project
// Windows implementation.
// ThreadWorkerBase class.
// Platform-specific worker thread implementation.

#ifndef NIRVANA_CORE_WINDOWS_THREADWORKER_H_
#define NIRVANA_CORE_WINDOWS_THREADWORKER_H_

#include "ThreadPoolable.h"
#include "ExecContext.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class ThreadWorker :
	public ThreadPoolable,
	public ExecContext
{
public:
	ThreadWorker (CompletionPort& completion_port) :
		ThreadPoolable (completion_port),
		ExecContext (nullptr)
	{}

	~ThreadWorker ()
	{}

	void attach (void* fiber)
	{
		ExecContext::port ().attach (fiber);
		Thread::attach ();
	}

	void detach ()
	{
		Thread::detach ();
	}

	virtual ExecContext* neutral_context ();

	static DWORD WINAPI thread_proc (ThreadWorker* _this);
};

}
}
}

#endif
