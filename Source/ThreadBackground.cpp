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
#include <ThreadBackground.inl>
#include <ExecDomain.h>
#include "Thread.inl"
#include <signal.h>

namespace Nirvana {
namespace Core {
namespace Port {

DWORD CALLBACK ThreadBackground::thread_proc (ThreadBackground* _this)
{
	Core::ThreadBackground& thread = static_cast <Core::ThreadBackground&> (*_this);
	Port::Thread::current (&thread);
	ExecContext& context = thread.neutral_context ().port ();
	try {
		context.convert_to_fiber ();
	} catch (...) {
		siginfo_t si;
		zero (si);
		si.si_excode = CORBA::SystemException::EC_NO_MEMORY;
		thread.exec_domain ()->on_crash (si);
		thread.on_thread_proc_end ();
		return 0;
	}

	do
		WaitForSingleObject (_this->event_, INFINITE);
	while (thread.execute ());
	
	context.convert_to_thread ();
	thread.on_thread_proc_end ();
	return 0;
}

ThreadBackground::ThreadBackground ()
{
	if (!(event_ = CreateEventW (nullptr, false, false, nullptr)))
		throw_NO_MEMORY ();
}

ThreadBackground::~ThreadBackground ()
{
	CloseHandle (event_);
}

void ThreadBackground::start ()
{
	Thread::create (this, Windows::BACKGROUND_THREAD_PRIORITY);
}

void ThreadBackground::resume () NIRVANA_NOEXCEPT
{
	verify (SetEvent (event_));
}

}
}
}
