// Nirvana project.
// Windows implementation.
// ThreadPoolable class. Thread object for ThreadPool.
#ifndef NIRVANA_CORE_THREADPOOLABLE_H_
#define NIRVANA_CORE_THREADPOOLABLE_H_

#include <Thread.h>
#include "ThreadInternal.h"
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
	{}

	template <class T>
	void create (T* p, int priority)
	{
		port ().create (p, priority);
	}

	void join ()
	{
		port ().join ();
	}

	static DWORD WINAPI thread_proc (ThreadPoolable* _this);

private:
	CompletionPort& completion_port_;
};

}
}
}

#endif
