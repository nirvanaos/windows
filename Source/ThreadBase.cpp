#include "ThreadInternal.h"
#include <CORBA/Exception.h>

namespace Nirvana {
namespace Core {
namespace Port {

thread_local Thread* Thread::current_;

void Thread::create (PTHREAD_START_ROUTINE thread_proc, void* param, int priority)
{
	assert (!handle_);
	handle_ = CreateThread (nullptr, Windows::NEUTRAL_FIBER_STACK_SIZE, thread_proc, param, 0, nullptr);
	if (!handle_)
		throw_INTERNAL ();
	verify (SetThreadPriority (handle_, priority));
}

}
}
}
