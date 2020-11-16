// Nirvana project.
// Windows implementation.
// Worker threads.

#include "WorkerThreads.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void WorkerThreads::run (Runnable& startup, DeadlineTime deadline)
{
	try {
		// Create other worker threads
		for (ThreadType* p = threads () + 1, *end = threads () + thread_count (); p != end; ++p) {
			p->port ().create ();
		}

		// Run main
		threads ()->port ().run_main (startup, deadline);

	} catch (...) {
		Base::terminate ();
		throw;
	}
	// Wait termination of all other worker threads
	for (ThreadType* p = threads () + 1, *end = threads () + thread_count (); p != end; ++p) {
		p->port ().join ();
	}
}

}
}
}
