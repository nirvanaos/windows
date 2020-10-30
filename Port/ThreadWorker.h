// Nirvana project
// Windows implementation.
// Thread class.
// Platform-specific worker thread implementation.

#ifndef NIRVANA_CORE_PORT_THREAD_H_
#define NIRVANA_CORE_PORT_THREAD_H_

#include "../Source/ThreadPoolable.h"

namespace Nirvana {
namespace Core {

class Runnable;

namespace Port {

class Thread :
	public Windows::ThreadPoolable
{
public:
	static Core::Thread* current ()
	{
		return current_;
	}

	Thread (Windows::CompletionPort& completion_port) :
		ThreadPoolable (completion_port)
	{}

	~Thread ();

	void run_main (Runnable& startup, DeadlineTime deadline);
	void create ();

private:
	friend class ThreadBase;
	static unsigned long __stdcall thread_proc (Thread* _this);
	static void __stdcall main_fiber_proc (void* p);
	struct MainFiberParam;
};

}
}
}

#endif
