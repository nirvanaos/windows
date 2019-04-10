#include "PortThread.h"
#include <CORBA/Exception.h>

namespace Nirvana {
namespace Core {
namespace Port {

DWORD Thread::tls_current_;

void Thread::create (SIZE_T stack_size, LPTHREAD_START_ROUTINE thread_proc, void* param, int priority)
{
	handle_ = CreateThread (nullptr, stack_size, thread_proc, param, 0, nullptr);
	if (!handle_)
		throw ::CORBA::INITIALIZE ();
	verify (SetThreadPriority (handle_, priority));
}

}
}
}
