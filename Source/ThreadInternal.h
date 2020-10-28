#ifndef NIRVANA_CORE_PORT_THREADINTERNAL_H_
#define NIRVANA_CORE_PORT_THREADINTERNAL_H_

#include <core.h>
#include "../Port/Thread.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

inline void Thread::attach ()
{
	assert (!handle_);
	verify (DuplicateHandle (GetCurrentProcess (), GetCurrentThread (), GetCurrentProcess (), &handle_, 0, FALSE, DUPLICATE_SAME_ACCESS));
	current_ = this;
}

inline void Thread::detach ()
{
	if (handle_) {
		assert (GetThreadId (handle_) == GetCurrentThreadId ());
		current_ = nullptr;
		CloseHandle (handle_);
		handle_ = nullptr;
	}
}

inline void Thread::join () const
{
	if (handle_)
		WaitForSingleObject (handle_, INFINITE);
}

inline void Thread::thread_proc ()
{
	current_ = this;
}

}
}
}

#endif
