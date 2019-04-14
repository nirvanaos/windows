// Nirvana project
// Windows implementation.
// ThreadWorkerBase class.

#include "ThreadWorker.h"

namespace Nirvana {
namespace Core {
namespace Windows {

DWORD WINAPI ThreadWorker::thread_proc (ThreadWorker* _this)
{
	_this->port ().convert_to_fiber ();
	_this->context (_this);
	ThreadPoolable::thread_proc (_this);
	_this->port ().convert_to_thread ();
	return 0;
}

ExecContext* ThreadWorker::neutral_context ()
{
	return this;
}

}
}
}
