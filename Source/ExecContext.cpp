#include <ExecDomain.h>
#include <Thread.h>
#include "../Port/Memory.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

unsigned long ExecContext::current_;

ExecContext::ExecContext (bool neutral) :
	fiber_ (nullptr)
{
	if (!neutral) {
		fiber_ = CreateFiber (0, (LPFIBER_START_ROUTINE)fiber_proc, static_cast <Core::ExecContext*> (this));
		if (!fiber_)
			throw CORBA::NO_MEMORY ();
	}
}

void __stdcall ExecContext::fiber_proc (Core::ExecContext* context)
{
	if (context)
		current (context);
	assert (current ()); // current () can be called in the special constructor for the main fiber.
	for (;;) {
		Core::Thread& thread = Core::Thread::current ();
		ExecDomain* ed = thread.exec_domain ();
		if (!ed)
			break; // This is for main thread only. The fiber procedure never completes.
		DWORD exc;
		__try {
			ed->execute_loop ();
		} __except (exc = GetExceptionCode (), EXCEPTION_EXECUTE_HANDLER) {
			ed->on_exec_domain_crash (CORBA::SystemException::EC_UNKNOWN);
		}
	}
}

void ExecContext::convert_to_fiber ()
{
	assert (!fiber_);
	// If dwFlags parameter is zero, the floating - point state on x86 systems is not switched and data 
	// can be corrupted if a fiber uses floating - point arithmetic.
	// This causes faster switching. Neutral execution context does not use floating - point arithmetic.
	if (!(fiber_ = ConvertThreadToFiberEx (nullptr, 0)))
		throw_NO_MEMORY ();

	current (static_cast <Core::ExecContext*> (this));
}

}
}
}
