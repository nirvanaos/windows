#ifndef NIRVANA_CORE_WINDOWS_THREAD_INL_
#define NIRVANA_CORE_WINDOWS_THREAD_INL_

#include <core.h>
#include "../Port/Thread.h"
#include "ExecContext.inl"

namespace Nirvana {
namespace Core {
namespace Port {

inline
void Thread::initialize ()
{
	current_ = TlsAlloc ();
	ExecContext::initialize ();
}

inline
void Thread::terminate ()
{
	ExecContext::terminate ();
	TlsFree (current_);
}

inline
void Thread::current (Core::Thread& core_thread)
{
	TlsSetValue (current_, &core_thread);
}

inline
Thread::~Thread ()
{
	if (handle_)
		CloseHandle (handle_);
}

inline
void Thread::join () const
{
	if (handle_)
		WaitForSingleObject (handle_, INFINITE);
}

}
}
}

#endif
