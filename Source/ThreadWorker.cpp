// Nirvana project
// Windows implementation.
// ThreadWorkerBase class.

#include "ThreadWorkerInternal.h"
#include "ExecContextInternal.h"

namespace Nirvana {
namespace Core {
namespace Port {

struct ThreadWorker::MainFiberParam
{
	Core_var <ExecDomain> main_domain;
	Windows::ThreadPoolable* worker_thread;
	Runnable* startup;
	DeadlineTime deadline;
};

DWORD WINAPI ThreadWorker::thread_proc (ThreadWorker* _this)
{
	Core::Thread& thread = static_cast <Core::ThreadWorker&> (*_this);
	current_ = &thread;
	thread.neutral_context ().port ().convert_to_fiber ();
	ThreadPoolable::thread_proc (_this);
	thread.neutral_context ().port ().convert_to_thread ();
	return 0;
}

void CALLBACK ThreadWorker::main_fiber_proc (void* p)
{
	MainFiberParam* param = (MainFiberParam*)p;
	ExecDomain* main_domain = param->main_domain;
	param->main_domain = nullptr; // Place my fiber to pool for reuse
	// Schedule startup runnable
	ExecDomain::async_call (*param->startup, param->deadline, nullptr, nullptr);
	ThreadPoolable::thread_proc (param->worker_thread);
	// Switch back to main fiber.
	main_domain->switch_to ();
}

void ThreadWorker::create ()
{
	Windows::ThreadPoolable::create (this, Windows::WORKER_THREAD_PRIORITY);
}

void ThreadWorker::run_main (Runnable& startup, DeadlineTime deadline)
{
	// Convert main thread to fiber
	void* main_fiber = ConvertThreadToFiber (nullptr);
	if (!main_fiber)
		throw CORBA::NO_MEMORY ();

	MainFiberParam param;
	// Convert main thread context into execution domain
	param.main_domain = ExecDomain::create (main_fiber);

	// Save for detach
	ExecDomain& main_domain = *param.main_domain;

	// Create fiber for neutral context
	param.worker_thread = this;
	param.startup = &startup;
	param.deadline = deadline;

	void* worker_fiber = CreateFiber (Windows::NEUTRAL_FIBER_STACK_SIZE, main_fiber_proc, &param);
	if (!worker_fiber)
		throw_NO_MEMORY ();

	Core::Thread& thread = static_cast <Core::ThreadWorker&> (*this);
	thread.neutral_context ().port ().attach (worker_fiber);
	current_ = &thread;

	// Set main thread priority to WORKER_THREAD_PRIORITY
	int prio = GetThreadPriority (GetCurrentThread ());
	SetThreadPriority (GetCurrentThread (), Windows::WORKER_THREAD_PRIORITY);

	// Switch to neutral context and run main_fiber_proc
	thread.context (main_domain);
	thread.neutral_context ().switch_to ();

	// Do fiber_proc for this worker thread
	ExecContext::fiber_proc (nullptr);

	// Restore priority and release resources
	SetThreadPriority (GetCurrentThread (), prio);
	ConvertFiberToThread ();

	main_domain.port ().detach ();	// Prevent DeleteFiber()
}

}
}
}
