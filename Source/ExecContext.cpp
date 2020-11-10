#include <ExecDomain.h>
#include <Thread.h>
#include "../Port/ProtDomainMemory.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

const DWORD EXCEPTION_ABORT = 1;

ExecContext::ExecContext (bool neutral) :
	fiber_ (nullptr)
{
	if (!neutral) {
		fiber_ = CreateFiber (0, fiber_proc, nullptr);
		if (!fiber_)
			throw CORBA::NO_MEMORY ();
	}
}

void __stdcall ExecContext::fiber_proc (void*)
{
	for (;;) { // The fiber procedure never completes.
		Core::Thread& thread = Core::Thread::current ();
		ExecDomain* ed = thread.exec_domain ();
		assert (ed);
		DWORD exc;
		__try {
			ed->execute_loop ();
		} __except (exc = GetExceptionCode (), EXCEPTION_EXECUTE_HANDLER) {
			thread.exec_domain (nullptr);
			ed->on_crash (EXCEPTION_ABORT == exc ? CORBA::SystemException::EC_INTERNAL : CORBA::SystemException::EC_UNKNOWN);
		}
	}
}

void ExecContext::abort ()
{
	RaiseException (EXCEPTION_ABORT, EXCEPTION_NONCONTINUABLE_EXCEPTION, 0, nullptr);
}

}
}
}
