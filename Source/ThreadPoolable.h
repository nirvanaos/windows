// Nirvana project.
// Windows implementation.
// ThreadPoolable class. Thread object for ThreadPool.
#ifndef NIRVANA_CORE_THREADPOOLABLE_H_
#define NIRVANA_CORE_THREADPOOLABLE_H_

#include <Thread.h>
#include "ThreadPool.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class ThreadPoolable :
	public Core::Thread
{
public:
	ThreadPoolable (CompletionPort& completion_port, int priority) :
		completion_port_ (completion_port)
	{
		ThreadWindows::create (this, NEUTRAL_FIBER_STACK_SIZE, priority);
	}

	~ThreadPoolable ()
	{
		join ();
	}

	static DWORD WINAPI thread_proc (ThreadPoolable* _this);

private:
	CompletionPort& completion_port_;
};

}
}
}

#endif
