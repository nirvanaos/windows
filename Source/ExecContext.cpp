/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include <ExecDomain.h>
#include <Thread.h>
#include "ExecContext.inl"
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

NIRVANA_NORETURN void ExecContext::abort ()
{
	RaiseException (STATUS_ABORT, EXCEPTION_NONCONTINUABLE, 0, nullptr);
}

}
}
}
