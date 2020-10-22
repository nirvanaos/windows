#include <ExecDomain.h>
#include "../Port/ProtDomainMemory.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

void __stdcall ExecContext::fiber_proc (void*)
{
	ExecDomain* ed = Core::Thread::current ().execution_domain ();
	assert (ed);
	__try {
		ed->execute_loop ();
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		ed->on_crash ();
	}
}

}
}
}
