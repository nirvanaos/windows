#ifndef NIRVANA_CORE_PORT_WORKER_SEMAPHORE_H_
#define NIRVANA_CORE_PORT_WORKER_SEMAPHORE_H_

#include "../Port/SystemInfo.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerSlave;

class WorkerSemaphore
{
public:
	static unsigned int thread_count ()
	{
		return Port::SystemInfo::hardware_concurrency ();
	}

	void thread_proc (SchedulerSlave& scheduler);

	WorkerSemaphore ()
	{
		handles_ [0] = nullptr;
		verify (handles_ [1] = CreateEventW (nullptr, true, FALSE, nullptr));
	}

	~WorkerSemaphore ()
	{
		for (HANDLE* p = handles_; p != std::end (handles_); ++p) {
			HANDLE h = p;
			if (h)
				CloseHandle (h);
		}
	}

	void semaphore (HANDLE sem)
	{
		if (handles_ [WORKER_SEMAPHORE])
			CloseHandle (handles_ [WORKER_SEMAPHORE]);
		handles_ [WORKER_SEMAPHORE] = sem;
	}

	HANDLE semaphore ()
	{
		assert (handles_ [WORKER_SEMAPHORE]);
		return handles_ [WORKER_SEMAPHORE];
	}

	void start ()
	{
		assert (handles_ [WORKER_SEMAPHORE]);
		ResetEvent (handles_ [SHUTDOWN_EVENT]);
	}

	void terminate () NIRVANA_NOEXCEPT
	{
		SetEvent (handles_ [SHUTDOWN_EVENT]);
	}

private:
	enum Handle
	{
		WORKER_SEMAPHORE,
		SHUTDOWN_EVENT,

		HANDLE_COUNT
	};

	HANDLE handles_ [HANDLE_COUNT];
};

}
}
}

#endif
