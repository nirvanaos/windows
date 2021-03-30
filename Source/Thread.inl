/// \file
/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
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
#ifndef NIRVANA_CORE_WINDOWS_THREAD_INL_
#define NIRVANA_CORE_WINDOWS_THREAD_INL_

#include <core.h>
#include "../Port/Thread.h"
#include "ExecContext.inl"

namespace Nirvana {
namespace Core {
namespace Port {

inline
void Thread::initialize ()
{
	current_ = TlsAlloc ();
	ExecContext::initialize ();
}

inline
void Thread::terminate ()
{
	ExecContext::terminate ();
	TlsFree (current_);
}

inline
void Thread::current (Core::Thread* core_thread)
{
	TlsSetValue (current_, core_thread);
}

inline
Thread::~Thread ()
{
	if (handle_)
		CloseHandle (handle_);
}

inline
void Thread::join () const
{
	if (handle_)
		WaitForSingleObject (handle_, INFINITE);
}

}
}
}

#endif
