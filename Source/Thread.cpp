#include "ThreadBaseInternal.h"
#include <CORBA/Exception.h>

namespace Nirvana {
namespace Core {
namespace Windows {

thread_local Core::Thread* ThreadBase::current_;

void ThreadBase::create (PTHREAD_START_ROUTINE thread_proc, void* param, int priority)
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
