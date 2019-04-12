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
	ThreadPoolable (CompletionPort& completion_port) :
		completion_port_ (completion_port)
	{
	}

	void create (int priority)
	{
		Port::Thread::create (this, NEUTRAL_FIBER_STACK_SIZE, priority);
	}

	void join ()
	{
		Thread::join ();
	}

	static DWORD WINAPI thread_proc (ThreadPoolable* _this);

private:
	CompletionPort& completion_port_;
};

}
}
}

#endif
