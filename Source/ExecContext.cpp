#include <ExecDomain.h>
#include <CORBA/Exception.h>
#include "ThreadMemory.h"
#include "../Port/config.h"

namespace Nirvana {
namespace Core {
namespace Port {

void CALLBACK ExecContext::fiber_proc (void*)
{
	if (SHARE_STACK) {
		Windows::ThreadMemory tm;
		Core::Thread::current ().execution_domain ()->execute_loop ();
	} else {
		Core::Thread::current ().execution_domain ()->execute_loop ();
	}
}

}
}
}
