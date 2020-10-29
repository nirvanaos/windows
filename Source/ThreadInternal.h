#ifndef NIRVANA_CORE_PORT_THREADINTERNAL_H_
#define NIRVANA_CORE_PORT_THREADINTERNAL_H_

#include <core.h>
#include "../Port/ThreadBase.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

inline ThreadBase::~ThreadBase ()
{
	if (handle_)
		CloseHandle (handle_);
}

inline void ThreadBase::attach ()
{
	assert (!handle_);
	verify (DuplicateHandle (GetCurrentProcess (), GetCurrentThread (), GetCurrentProcess (), &handle_, 0, FALSE, DUPLICATE_SAME_ACCESS));
}

inline void ThreadBase::detach ()
{
	if (handle_) {
		assert (GetThreadId (handle_) == GetCurrentThreadId ());
		current_ = nullptr;
		CloseHandle (handle_);
		handle_ = nullptr;
	}
}

inline void ThreadBase::join () const
{
	if (handle_)
		WaitForSingleObject (handle_, INFINITE);
}

}
}
}

#endif
