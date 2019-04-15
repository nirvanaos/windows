// Nirvana project.
// Windows implementation.
// Worker threads.

#include "WorkerThreads.h"
#include <CORBA/Exception.h>
#include <SysScheduler.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void WorkerThreads::run (Runnable_ptr startup, DeadlineTime deadline)
{
	MainFiberParam param;
	
	void* my_fiber = ConvertThreadToFiber (nullptr);
	if (!my_fiber)
		throw CORBA::NO_MEMORY ();

	// Convert main thread context into execution domain
	param.main_context = new ExecDomain (my_fiber);

	param.worker_thread = threads ();
	param.startup = startup;
	param.deadline = deadline;

	void* worker_fiber = CreateFiber (NEUTRAL_FIBER_STACK_SIZE, main_fiber_proc, &param);
	if (!worker_fiber)
		throw CORBA::NO_MEMORY ();

	threads ()->attach (worker_fiber);
	for (ThreadWorker* p = threads () + 1, *end = threads () + thread_count (); p != end; ++p) {
		p->port ().create (p, WORKER_THREAD_PRIORITY);
	}

	int prio = GetThreadPriority (GetCurrentThread ());
	SetThreadPriority (GetCurrentThread (), WORKER_THREAD_PRIORITY);
	threads ()->neutral_context ()->switch_to ();

	Port::ExecContext::fiber_proc (nullptr);

	threads ()->detach ();

	SetThreadPriority (GetCurrentThread (), prio);

	ConvertFiberToThread ();

	param.main_context->port ().detach ();	// Prevent DeleteFiber()

	ThreadPool <ThreadWorker>::terminate ();
}

void CALLBACK WorkerThreads::main_fiber_proc (void* p)
{
	MainFiberParam* param = (MainFiberParam*)p;
	param->main_context->_remove_ref (); // Place my fiber to pool for reuse
	ExecDomain::async_call (param->startup, param->deadline, nullptr);
	ThreadPoolable::thread_proc (((MainFiberParam*)param)->worker_thread);
	param->main_context->switch_to ();
}

void WorkerThreads::shutdown ()
{
	CompletionPort::terminate ();
}

}
}
}
