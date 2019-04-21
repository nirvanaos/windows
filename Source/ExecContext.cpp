#include <ExecDomain.h>
#include <CORBA/Exception.h>
#include "ThreadMemory.h"
#include "../Port/config.h"
#include "../Port/ProtDomainMemory.h"

namespace Nirvana {
namespace Core {
namespace Port {

void CALLBACK ExecContext::fiber_proc (void*)
{
	_set_se_translator (&ProtDomainMemory::se_translator);
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
