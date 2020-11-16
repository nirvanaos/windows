// Nirvana project.
// Windows implementation.
// ThreadPostman class. Thread object for ThreadPool.

#include "ThreadPostman.h"
#include "CompletionPort.h"

namespace Nirvana {
namespace Core {
namespace Windows {

DWORD __stdcall ThreadPostman::thread_proc (ThreadPostman* _this)
{
	_this->completion_port_.thread_proc ();
	return 0;
}

}
}
}