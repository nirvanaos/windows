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
#include "Thread.inl"

namespace Nirvana {
namespace Core {
namespace Port {

unsigned long Thread::current_;

void Thread::create (PTHREAD_START_ROUTINE thread_proc, void* param, int priority)
{
	assert (!handle_);
	handle_ = CreateThread (nullptr, Windows::NEUTRAL_FIBER_STACK_RESERVE, thread_proc, param,
		STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
	if (!handle_)
		throw_NO_MEMORY ();
	if (THREAD_PRIORITY_NORMAL != priority)
		verify (SetThreadPriority (handle_, priority));
}

}
}
}
