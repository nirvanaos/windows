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
#include "../Port/SystemInfo.h"
#include "win32.h"

namespace Nirvana {
namespace Core {

namespace Windows {

class TimerPool
{
public:
	void initialize () noexcept
	{
		InitializeThreadpoolEnvironment (&callback_environ_);
		thread_pool_ = CreateThreadpool (nullptr);
		assert (thread_pool_);

		TP_POOL_STACK_INFORMATION si { NEUTRAL_FIBER_STACK_SIZE, NEUTRAL_FIBER_STACK_SIZE };
		verify (SetThreadpoolStackInformation (thread_pool_, &si));
		SetThreadpoolThreadMaximum (thread_pool_, Port::SystemInfo::hardware_concurrency ());
		verify (SetThreadpoolThreadMinimum (thread_pool_, 1));

		SetThreadpoolCallbackPool (&callback_environ_, thread_pool_);
	}

	void terminate () noexcept
	{
		CloseThreadpool (thread_pool_);
	}

	TP_TIMER* timer_create (Port::Timer& timer)
	{
		TP_TIMER* p = CreateThreadpoolTimer (&timer_callback, &timer, &callback_environ_);
		if (!p)
			throw_NO_MEMORY ();
		return p;
	}

private:
	static void CALLBACK timer_callback (TP_CALLBACK_INSTANCE*, void* context, TP_TIMER*);

private:
	TP_POOL* thread_pool_;
	TP_CALLBACK_ENVIRON callback_environ_;
};

void CALLBACK TimerPool::timer_callback (TP_CALLBACK_INSTANCE*, void* context, TP_TIMER*)
{
	((Port::Timer*)context)->signal ();
}

static TimerPool timer_pool;

}

using Windows::timer_pool;

namespace Port {

void Timer::initialize () noexcept
{
	timer_pool.initialize ();
}

void Timer::terminate () noexcept
{
	timer_pool.terminate ();
}

Timer::Timer () :
	timer_ (timer_pool.timer_create (*this))
{}

Timer::~Timer ()
{
	SetThreadpoolTimer (timer_, nullptr, 0, 0);
	WaitForThreadpoolTimerCallbacks (timer_, true);
	CloseThreadpoolTimer (timer_);
}

void Timer::set (unsigned flags, TimeBase::TimeT due_time, TimeBase::TimeT period)
{
	if (period > (TimeBase::TimeT)std::numeric_limits <DWORD>::max ())
		throw_BAD_PARAM ();
	LARGE_INTEGER dt;
	if (flags & Core::Timer::TIMER_ABSOLUTE)
		dt.QuadPart = due_time - Windows::WIN_TIME_OFFSET_SEC * TimeBase::SECOND;
	else {
		if (due_time > (TimeBase::TimeT)std::numeric_limits <LONGLONG>::max ())
			throw_BAD_PARAM ();
		dt.QuadPart = -(LONGLONG)due_time;
	}
	SetThreadpoolTimer (timer_, (PFILETIME)&dt, (DWORD)period, 0);
}

void Timer::cancel () NIRVANA_NOEXCEPT
{
	SetThreadpoolTimer (timer_, nullptr, 0, 0);
}

}
}
}
