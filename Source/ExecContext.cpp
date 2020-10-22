#include <ExecDomain.h>
#include "../Port/ProtDomainMemory.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

void __stdcall ExecContext::fiber_proc (void*)
{
	// TODO: SEH handler
	Core::Thread::current ().execution_domain ()->execute_loop ();
}

}
}
}
