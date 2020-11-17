#include "SchedulerSlave.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void WorkerSemaphore::thread_proc (SchedulerSlave& scheduler)
{
	while (WaitForMultipleObjects ((DWORD)std::size (handles_), handles_, FALSE, INFINITE) == WAIT_OBJECT_0) {
		scheduler.execute ();
	}
}

}
}
}
