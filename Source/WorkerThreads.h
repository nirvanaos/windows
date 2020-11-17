// Nirvana project.
// Windows implementation.
// Worker threads.

#ifndef NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_
#define NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_

#include "ThreadWorker.h"
#include "ThreadPool.h"

namespace Nirvana {
namespace Core {
namespace Windows {

template <class Master>
class WorkerThreads :
	public ThreadPool <Master, ThreadWorker>
{
	typedef ThreadPool <Master, ThreadWorker> Pool;
public:
	void run (Runnable& startup, DeadlineTime deadline)
	{
		Master::start ();
		try {

			// Create other worker threads
			for (ThreadWorker* p = Pool::threads () + 1,
				*end = Pool::threads () + Pool::thread_count (); p != end; ++p) {
				p->create ();
			}

			// Run main
			Pool::threads ()->run_main (startup, deadline);

		} catch (...) {
			terminate ();
			throw;
		}
		terminate ();
	}

	void shutdown ()
	{
		Master::terminate ();
	}
};

}
}
}

#endif
