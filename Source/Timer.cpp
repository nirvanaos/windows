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

#include <Timer.h>
#include <limits>
#include "MessageBroker.h"

namespace Nirvana {
namespace Core {
namespace Port {


Timer::~Timer ()
{
	if (handle_)
		CloseHandle (handle_);
}

void Timer::completed (_OVERLAPPED* ovl, uint32_t size, uint32_t error) NIRVANA_NOEXCEPT
{
	if (!period_ && handle_) {
		HANDLE h = handle_;
		handle_ = nullptr;
		verify (CloseHandle (h));
	}
	signal ();
}

void Timer::set (unsigned flags, TimeBase::TimeT due_time, TimeBase::TimeT period)
{
	if (period > (TimeBase::TimeT)std::numeric_limits <LONG>::max ())
		throw_BAD_PARAM ();
	LARGE_INTEGER dt;
	if (flags & Core::Timer::TIMER_ABSOLUTE)
		dt.QuadPart = due_time - Windows::WIN_TIME_OFFSET_SEC * TimeBase::SECOND;
	else {
		if (due_time > (TimeBase::TimeT)std::numeric_limits <LONGLONG>::max ())
			throw_BAD_PARAM ();
		dt.QuadPart = -(LONGLONG)due_time;
	}
	if (!handle_ && !(handle_ = CreateWaitableTimerW (nullptr, false, nullptr)))
		throw_NO_MEMORY (GetLastError ());
	Windows::MessageBroker::completion_port ().add_receiver (handle_, *this);
	verify (SetWaitableTimer (handle_, &dt, period_ = (LONG)period, nullptr, nullptr, flags & Core::Timer::TIMER_WAKEUP));
}

void Timer::cancel () NIRVANA_NOEXCEPT
{
	if (handle_) {
		HANDLE h = handle_;
		handle_ = nullptr;
		verify (CancelWaitableTimer (h));
		verify (CloseHandle (h));
	}
}

}
}
}
