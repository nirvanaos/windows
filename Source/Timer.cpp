/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2025 Igor Popov.
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
#include <Timer.h>

namespace Nirvana {
namespace Core {

namespace Windows {

StaticallyAllocated <Timer::Global> Timer::global_;

inline void Timer::initialize ()
{
	global_.construct ();
}

inline void Timer::terminate () noexcept
{
	global_.destruct ();
}

Timer::~Timer ()
{
	SetEvent (handles_ [HANDLE_DESTRUCT]);
	Thread::join ();
	CloseHandle (handles_ [HANDLE_TIMER]);
	CloseHandle (handles_ [HANDLE_DESTRUCT]);
}

unsigned long __stdcall Timer::thread_proc (Timer* _this)
{
	DWORD result;
	while ((WAIT_OBJECT_0 + HANDLE_TIMER) == (result = WaitForMultipleObjects (HANDLE_CNT, _this->handles_, false, INFINITE))) {
		_this->facade_->signal ();
	}

	if (WAIT_OBJECT_0 + HANDLE_TERMINATE == result)
		delete _this;

	return 0;
}

inline void Timer::set (const LARGE_INTEGER& due_time, LONG period, bool resume)
{
	if (!SetWaitableTimer (handles_ [HANDLE_TIMER], &due_time, period, nullptr, nullptr, resume))
		Windows::throw_last_error ();
}

inline void Timer::cancel () noexcept
{
	CancelWaitableTimer (handles_ [HANDLE_TIMER]);
}

}

namespace Port {

void Timer::initialize ()
{
	Windows::Timer::initialize ();
}

void Timer::terminate ()
{
	Windows::Timer::terminate ();
}

Timer::Timer () :
	timer_ (Windows::Timer::create (*this))
{}

Timer::~Timer ()
{
	timer_.release ();
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

	timer_.set (dt, period, false);
}

void Timer::cancel ()
{
	timer_.cancel ();
}

inline void Timer::signal () noexcept
{
	static_cast <Core::Timer&> (*this).port_signal ();
}

}
}
}
