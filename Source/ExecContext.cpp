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
	ExecDomain* ed = thread.exec_domain ();
	assert (ed);
	__try {
		ed->execute_loop ();
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		thread.exec_domain (nullptr);
		ed->on_crash ();
	}
}

ExecContext::ExecContext (bool neutral) :
	fiber_ (nullptr)
{
	if (!neutral) {
		fiber_ = CreateFiber (0, fiber_proc, nullptr);
		if (!fiber_)
			throw CORBA::NO_MEMORY ();
	}
}

}
}
}
