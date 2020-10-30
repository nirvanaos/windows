#ifndef NIRVANA_CORE_WINDOWS_THREADINTERNAL_H_
#define NIRVANA_CORE_WINDOWS_THREADINTERNAL_H_

#include <core.h>
#include "../Port/Thread.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

inline Thread::~Thread ()
{
	if (handle_)
		CloseHandle (handle_);
}

inline void Thread::join () const
{
	if (handle_)
		WaitForSingleObject (handle_, INFINITE);
}

}
}
}

#endif
