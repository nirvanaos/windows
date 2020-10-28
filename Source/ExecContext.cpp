#include <ExecDomain.h>
#include <Thread.h>
#include "../Port/ProtDomainMemory.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

void __stdcall ExecContext::fiber_proc (void*)
{
	Core::Thread& thread = Core::Thread::current ();
	ExecDomain* ed = thread.execution_domain ();
	assert (ed);
	__try {
		ed->execute_loop ();
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		thread.execution_domain (nullptr);
		ed->on_crash ();
	}
}

}
}
}
