// Nirvana project.
// Windows implementation.
// ThreadPoolable class. Thread object for ThreadPool.
#ifndef NIRVANA_CORE_THREADPOOLABLE_H_
#define NIRVANA_CORE_THREADPOOLABLE_H_

#include "ThreadBase.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class CompletionPort;

class ThreadPoolable :
	public ThreadBase
{
public:
	/// For template compatibility with Core::Tread
	ThreadPoolable& port ()
	{
		return *this;
	}

	ThreadPoolable (CompletionPort& completion_port) :
		completion_port_ (completion_port)
	{}

	static unsigned long __stdcall thread_proc (ThreadPoolable* _this);

private:
	CompletionPort& completion_port_;
};

}
}
}

#endif
