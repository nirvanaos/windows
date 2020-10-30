// Nirvana project
// Windows implementation.
// Port::ThreadWorker class.
// Platform-specific worker thread implementation.

#ifndef NIRVANA_CORE_PORT_THREADWORKER_H_
#define NIRVANA_CORE_PORT_THREADWORKER_H_

#include "../Source/ThreadPoolable.h"

namespace Nirvana {
namespace Core {

class Runnable;

namespace Port {

class ThreadWorker :
	public Windows::ThreadPoolable
{
public:
	ThreadWorker (Windows::CompletionPort& completion_port) :
		ThreadPoolable (completion_port)
	{}

	~ThreadWorker ();

	void run_main (Runnable& startup, DeadlineTime deadline);
	void create ();

private:
	friend class Thread;
	static unsigned long __stdcall thread_proc (ThreadWorker* _this);
	static void __stdcall main_fiber_proc (void* p);
	struct MainFiberParam;
};

}
}
}

#endif
