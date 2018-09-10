// Nirvana project
// Windows implementation.
// ThreadWorkerBase class.

#include "../ThreadWorker.h"
#include "../ProtDomain.h"

namespace Nirvana {
namespace Core {
namespace Windows {

DWORD WINAPI ThreadWorkerBase::thread_proc (void* param)
{
	ThreadWorkerBase* _this = (ThreadWorkerBase*)param;
	_this->thread_init ();

	verify (SetThreadPriority (_this->handle (), WORKER_THREAD_PRIORITY));
//	ProtDomain::singleton ().worker_thread_proc (*static_cast <ThreadWorker*> (_this));

	_this->thread_term ();

	return 0;
}

}
}
}
