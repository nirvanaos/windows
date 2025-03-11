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
#include "pch.h"
#include "Timer.h"
#include "Handle.h"

namespace Nirvana {
namespace Core {
namespace Windows {

StaticallyAllocated <Timer::Global> Timer::global_;

void Timer::initialize ()
{
	global_.construct ();
}

void Timer::terminate () noexcept
{
	global_.destruct ();
}

Timer::~Timer ()
{
	CloseHandle (handles_ [HANDLE_TIMER]);
	CloseHandle (handles_ [HANDLE_DESTRUCT]);
}

void Timer::set (unsigned flags, TimeBase::TimeT due_time, TimeBase::TimeT period)
{
	period /= TimeBase::MILLISECOND;
	if (period > (TimeBase::TimeT)std::numeric_limits <LONG>::max ())
		throw_BAD_PARAM ();
	LARGE_INTEGER dt;
	if (flags & TIMER_ABSOLUTE)
		dt.QuadPart = due_time - Windows::WIN_TIME_OFFSET_SEC * TimeBase::SECOND;
	else {
		if (due_time > (TimeBase::TimeT)std::numeric_limits <LONGLONG>::max ())
			throw_BAD_PARAM ();
		dt.QuadPart = -(LONGLONG)due_time;
	}
	if (!SetWaitableTimer (handles_ [HANDLE_TIMER], &dt, (LONG)period, nullptr, nullptr, true))
		Windows::throw_last_error ();
}

void Timer::cancel () noexcept
{
	CancelWaitableTimer (handles_ [HANDLE_TIMER]);
}

unsigned long __stdcall Timer::thread_proc (Timer* _this)
{
	_this->completion_port_.thread_proc ();
	return 0;
}

}
}
}
