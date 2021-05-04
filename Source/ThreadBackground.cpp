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
#include <Legacy/ThreadBackground.h>
#include <ExecDomain.h>
#include "Thread.inl"

namespace Nirvana {
namespace Core {
namespace Port {

DWORD CALLBACK ThreadBackground::thread_proc (ThreadBackground* _this)
{
	Legacy::Core::ThreadBackground& thread = static_cast <Legacy::Core::ThreadBackground&> (*_this);
	Port::Thread::current (&thread);
	try {
		thread.neutral_context().port ().convert_to_fiber ();
	} catch (...) {
		thread.exec_domain ()->on_exec_domain_crash (CORBA::SystemException::EC_NO_MEMORY);
		thread.on_thread_proc_end ();
		return 0;
	}
	thread.exec_domain ()->switch_to ();
	Core::ExecContext::neutral_context_loop ();
	thread.neutral_context ().port ().convert_to_thread ();
	thread.on_thread_proc_end ();
	return 0;
}

ThreadBackground::ThreadBackground (bool process)
{
	if (!(event_ = CreateEventW (nullptr, 0, 0, nullptr)))
		throw_NO_MEMORY ();
	if (process)
		AllocConsole ();
}

ThreadBackground::~ThreadBackground ()
{
	CloseHandle (event_);
}

void ThreadBackground::create ()
{
	Thread::create (this, Windows::BACKGROUND_THREAD_PRIORITY);
}

void ThreadBackground::suspend ()
{
	WaitForSingleObject (event_, INFINITE);
}

void ThreadBackground::resume ()
{
	verify (SetEvent (event_));
}

/// Temparary boost thread priority above the worker threads priority.
void ThreadBackground::priority_boost ()
{
	SetThreadPriority (handle_, Windows::BACKGROUND_THREAD_PRIORITY_BOOSTED);
}

void ThreadBackground::priority_restore ()
{
	SetThreadPriority (handle_, Windows::BACKGROUND_THREAD_PRIORITY);
}

}
}
}
