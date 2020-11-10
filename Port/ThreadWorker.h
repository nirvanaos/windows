// Nirvana project
// Windows implementation.
// Port::ThreadWorker class.
// Platform-specific worker thread implementation.

#ifndef NIRVANA_CORE_PORT_THREADWORKER_H_
#define NIRVANA_CORE_PORT_THREADWORKER_H_

#include "Thread.h"

namespace Nirvana {
namespace Core {

class Runnable;

namespace Windows {
class TaskMaster;
}

namespace Port {

class ThreadWorker :
	public Thread
{
public:
	void run_main (Runnable& startup, DeadlineTime deadline);
	void create ();

protected:
	ThreadWorker (Windows::TaskMaster& master) :
		master_ (master)
	{}

	~ThreadWorker ();

private:
	friend class Thread;
	static unsigned long __stdcall thread_proc (ThreadWorker* _this);
	struct MainFiberParam;
	static void __stdcall main_fiber_proc (MainFiberParam* param);

private:
	Windows::TaskMaster& master_;
};

}
}
}

#endif
