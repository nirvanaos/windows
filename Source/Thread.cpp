#include "ThreadInternal.h"
#include <CORBA/Exception.h>

namespace Nirvana {
namespace Core {
namespace Port {

uint32_t Thread::tls_current_;

void Thread::create (size_t stack_size, PTHREAD_START_ROUTINE thread_proc, void* param, int priority)
{
	handle_ = CreateThread (nullptr, stack_size, thread_proc, param, 0, nullptr);
	if (!handle_)
		throw ::CORBA::INITIALIZE ();
	verify (SetThreadPriority (handle_, priority));
}

}
}
}
