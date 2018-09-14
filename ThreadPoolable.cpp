// Nirvana project.
// Windows implementation.
// ThreadPoolable class. Thread object for ThreadPool.

#include "ThreadPoolable.h"

namespace Nirvana {
namespace Core {
namespace Windows {

DWORD __stdcall Nirvana::Core::Windows::ThreadPoolable::thread_proc (ThreadPoolable* _this)
{
	ThreadWindows::thread_proc (_this);
	_this->completion_port_.thread_proc ();
	return 0;
}

}
}
}