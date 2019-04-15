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
	public ThreadPoolable
{
public:
	ThreadWorker (CompletionPort& completion_port) :
		ThreadPoolable (completion_port),
		neutral_context_ (nullptr)
	{}

	~ThreadWorker ()
	{}

	void attach (void* fiber)
	{
		neutral_context_.port ().attach (fiber);
		port ().attach ();
	}

	void detach ()
	{
		port ().detach ();
	}

	virtual ExecContext* neutral_context ();

	static DWORD WINAPI thread_proc (ThreadWorker* _this);

private:
	ExecContext neutral_context_;
};

}
}
}

#endif
