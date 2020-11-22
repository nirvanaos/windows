// Nirvana project
// Windows implementation.
// ThreadWorkerBase class.

#include "WorkerThreads.h"
#include "WorkerSemaphore.h"
#include "CompletionPort.h"
#include <ExecDomain.h>
#include "../Port/Scheduler.h"

namespace Nirvana {
namespace Core {
namespace Windows {

unsigned long __stdcall ThreadWorker::thread_proc (ThreadWorker* _this)
{
	Core::Thread& thread = static_cast <Core::Thread&> (*_this);
	Port::Thread::current (thread);
	thread.neutral_context ().port ().convert_to_fiber ();
	Port::Scheduler::worker_thread_proc ();
	thread.neutral_context ().port ().convert_to_thread ();
	return 0;
}

struct ThreadWorker::MainNeutralFiberParam
{
	Core_var <ExecDomain> main_domain;
};

void CALLBACK ThreadWorker::main_neutral_fiber_proc (MainNeutralFiberParam* param)
{
	Port::ExecContext::current (Core::Thread::current ().neutral_context ());
	ExecDomain* main_domain = param->main_domain;
	// Schedule startup runnable
	main_domain->spawn (nullptr);
	// Release main fiber to pool for reuse.
	param->main_domain.reset ();
	// Do worker thread proc.
	Port::Scheduler::worker_thread_proc ();
	// Switch back to main fiber.
	main_domain->switch_to ();
}

void ThreadWorker::run_main (Runnable& startup, DeadlineTime deadline)
{
	Core::Thread& thread = static_cast <Core::ThreadWorker&> (*this);
	Port::Thread::current (thread);

	// Convert main thread to fiber
	void* main_fiber = ConvertThreadToFiber (nullptr);
	if (!main_fiber)
		throw_NO_MEMORY ();

	MainNeutralFiberParam param;

	// Convert main thread context into execution domain
	param.main_domain = ExecDomain::create_main (deadline, startup, main_fiber);

	// Save for detach
	ExecDomain& main_domain = *param.main_domain;
	Port::ExecContext::current (main_domain);

	// Create fiber for neutral context
	void* worker_fiber = CreateFiber (Windows::NEUTRAL_FIBER_STACK_SIZE, (LPFIBER_START_ROUTINE)main_neutral_fiber_proc, &param);
	if (!worker_fiber)
		throw_NO_MEMORY ();

	thread.neutral_context ().port ().attach (worker_fiber);

#ifdef _DEBUG
	DWORD dbg_main_thread = GetCurrentThreadId ();
#endif

	// Set main thread priority to WORKER_THREAD_PRIORITY
	int prio = GetThreadPriority (GetCurrentThread ());
	SetThreadPriority (GetCurrentThread (), Windows::WORKER_THREAD_PRIORITY);

	// Switch to neutral context and run main_fiber_proc
	thread.neutral_context ().switch_to ();

	// Do fiber_proc for this worker thread
	Port::ExecContext::fiber_proc (nullptr);

	assert (dbg_main_thread == GetCurrentThreadId ());
	assert (!handle_); // Prevent join to self.

	// Restore priority and release resources
	SetThreadPriority (GetCurrentThread (), prio);
	ConvertFiberToThread ();

	main_domain.port ().detach ();	// Prevent DeleteFiber()
}

}
}
}
