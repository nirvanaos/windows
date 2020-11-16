#ifndef NIRVANA_CORE_PORT_TASKMASTER_H_
#define NIRVANA_CORE_PORT_TASKMASTER_H_

#include "SchedulerAbstract.h"
#include "../Port/SystemInfo.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class TaskMaster : public SchedulerAbstract
{
public:
	void thread_proc ()
	{
		while (WaitForMultipleObjects ((DWORD)std::size (handles_), handles_, FALSE, INFINITE) == WAIT_OBJECT_0) {
			execute ();
		}
	}

protected:
	TaskMaster ()
	{
		handles_ [0] = nullptr;
		verify (handles_ [1] = CreateEventW (nullptr, true, FALSE, nullptr));
	}

	~TaskMaster ()
	{
		for (HANDLE* p = handles_; p != std::end (handles_); ++p) {
			HANDLE h = p;
			if (h)
				CloseHandle (h);
		}
	}

	void start ()
	{
		if (!handles_ [WORKER_SEMAPHORE])
			verify (handles_ [WORKER_SEMAPHORE] = CreateSemaphoreW (nullptr, 0, (LONG)Port::g_system_info.hardware_concurrency (), nullptr));
		ResetEvent (handles_ [SHUTDOWN_EVENT]);
	}

	void terminate () NIRVANA_NOEXCEPT
	{
		SetEvent (handles_ [SHUTDOWN_EVENT]);
	}

	virtual void shutdown () NIRVANA_NOEXCEPT
	{
		terminate ();
	}

	void semaphore (HANDLE sem)
	{
		if (handles_ [WORKER_SEMAPHORE])
			CloseHandle (handles_ [WORKER_SEMAPHORE]);
		handles_ [WORKER_SEMAPHORE] = sem;
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
