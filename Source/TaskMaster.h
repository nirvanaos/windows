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
		verify (handles_ [0] = CreateSemaphoreW (nullptr, 0, (LONG)Port::g_system_info.hardware_concurrency (), nullptr));
		verify (handles_ [1] = CreateEventW (nullptr, true, FALSE, nullptr));
	}

	~TaskMaster ()
	{
		for (HANDLE* p = handles_; p != std::end (handles_); ++p) {
			CloseHandle (*p);
		}
	}

	void start ()
	{}

	void terminate () NIRVANA_NOEXCEPT
	{
		assert (false); // Must never be called.
	}

	virtual void shutdown () NIRVANA_NOEXCEPT
	{
		SetEvent (handles_ [SHUTDOWN_EVENT]);
	}

	HANDLE semaphore () const
	{
		return handles_ [WORKER_SEMAPHORE];
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
