#include "ThreadBase.h"
#include <ORB.h>

namespace Nirvana {
namespace Core {
namespace Windows {

DWORD ThreadBase::tls_current_;

void ThreadBase::create (LPTHREAD_START_ROUTINE thread_proc, void* param)
{
	handle_ = CreateThread (nullptr, NEUTRAL_FIBER_STACK_SIZE, thread_proc, param, 0, nullptr);
	if (!handle_)
		throw ::CORBA::INITIALIZE ();
}

}
}
}
