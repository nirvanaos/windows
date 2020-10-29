#ifndef NIRVANA_CORE_PORT_THREADBASEINTERNAL_H_
#define NIRVANA_CORE_PORT_THREADBASEINTERNAL_H_

#include <core.h>
#include "ThreadBase.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

inline ThreadBase::~ThreadBase ()
{
	if (handle_)
		CloseHandle (handle_);
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
