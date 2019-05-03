#ifndef NIRVANA_CORE_PORT_THREADINTERNAL_H_
#define NIRVANA_CORE_PORT_THREADINTERNAL_H_

#include "../Port/Thread.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

inline void Thread::initialize ()
{
	tls_current_ = TlsAlloc ();
}

inline void Thread::terminate ()
{
	TlsFree (tls_current_);
}

inline void Thread::attach ()
{
	assert (!handle_);
	BOOL ok = DuplicateHandle (GetCurrentProcess (), GetCurrentThread (), GetCurrentProcess (), &handle_, 0, FALSE, DUPLICATE_SAME_ACCESS);
	assert (ok);
	TlsSetValue (tls_current_, this);
}

inline void Thread::detach ()
{
	if (handle_) {
		assert (GetThreadId (handle_) == GetCurrentThreadId ());
		TlsSetValue (tls_current_, nullptr);
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
	TlsSetValue (tls_current_, this);
}

}
}
}

#endif
