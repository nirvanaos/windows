// Nirvana project.
// Windows implementation.
// Worker threads.

#include "WorkerThreads.h"
#include <CORBA/Exception.h>
#include <Scheduler.h>

namespace Nirvana {
namespace Core {
namespace Windows {

WorkerThreads::WorkerThreads ()
{
	Scheduler::initialize ();
	Port::Thread::initialize ();
	CompletionPort::start ();
}

WorkerThreads::~WorkerThreads ()
{
	Port::Thread::terminate ();
	Scheduler::terminate ();
}

void WorkerThreads::run (Runnable& startup, DeadlineTime deadline)
{
	MainFiberParam param;
	
	// Convert main thread to fiber
	void* my_fiber = ConvertThreadToFiber (nullptr);
	if (!my_fiber)
		throw CORBA::NO_MEMORY ();

	// Convert main thread context into execution domain
	param.main_context = new ExecDomain (my_fiber);

	// Create fiber for neutral context
	param.worker_thread = threads ();
	param.startup = &startup;
	param.deadline = deadline;

	void* worker_fiber = CreateFiber (NEUTRAL_FIBER_STACK_SIZE, main_fiber_proc, &param);
	if (!worker_fiber)
		throw CORBA::NO_MEMORY ();

	// Attach main thread as first worker thread in pool
	threads ()->attach (worker_fiber);

	// Create other worker threads
	for (ThreadWorker* p = threads () + 1, *end = threads () + thread_count (); p != end; ++p) {
		p->port ().create (p, WORKER_THREAD_PRIORITY);
	}

	// Set main thread priority to WORKER_THREAD_PRIORITY
	int prio = GetThreadPriority (GetCurrentThread ());
	SetThreadPriority (GetCurrentThread (), WORKER_THREAD_PRIORITY);

	// Switch to neutral context and run main_fiber_proc
	threads ()->neutral_context ()->switch_to ();

	// Do fiber_proc for this worker threads
	Port::ExecContext::fiber_proc (nullptr);

	// Detach main thread from pool
	threads ()->detach ();

	// Restore priority and release resources
	SetThreadPriority (GetCurrentThread (), prio);
	ConvertFiberToThread ();

	param.main_context->port ().detach ();	// Prevent DeleteFiber()

	// Wait termination of all other worker threads
	ThreadPool <ThreadWorker>::terminate ();
}

void CALLBACK WorkerThreads::main_fiber_proc (void* p)
{
	MainFiberParam* param = (MainFiberParam*)p;
	param->main_context->_remove_ref (); // Place my fiber to pool for reuse
	ExecDomain::async_call (*param->startup, param->deadline, nullptr);
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
