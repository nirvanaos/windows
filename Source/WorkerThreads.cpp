// Nirvana project.
// Windows implementation.
// Worker threads.

#include "WorkerThreads.h"
#include <CORBA/Exception.h>
#include <Scheduler.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void WorkerThreads::run (Runnable& startup, DeadlineTime deadline)
{
	// Create other worker threads
	for (ThreadWorker* p = threads () + 1, *end = threads () + thread_count (); p != end; ++p) {
		p->port ().create ();
	}

	// Run main
	threads ()->port ().run_main (startup, deadline);

	// Wait termination of all other worker threads
	ThreadPool <ThreadWorker>::terminate ();
}

void WorkerThreads::shutdown ()
{
	CompletionPort::terminate ();
}

}
}
}
