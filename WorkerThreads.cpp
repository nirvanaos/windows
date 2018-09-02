// Nirvana project.
// Windows implementation.
// Worker threads.

#include "WorkerThreads.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void WorkerThreads::close ()
{
	for (auto buffer = buffers_.begin (); buffer != buffers_.end (); ++buffer) {
		if (buffer->enqueued)
			CancelIoEx (run_mailslot_, &*buffer);
	}
	if (completion_port_)
		CloseHandle (completion_port_);
	for (auto thread = threads_.begin (); thread != threads_.end (); ++thread)
		thread->join ();
	if (free_cores_semaphore_)
		CloseHandle (free_cores_semaphore_);
	if (INVALID_HANDLE_VALUE != scheduler_mailslot_)
		CloseHandle (scheduler_mailslot_);
	if (INVALID_HANDLE_VALUE != run_mailslot_)
		CloseHandle (run_mailslot_);
}

}
}
}
