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
#include "../Port/Timer.h"
#include "../Port/SystemInfo.h"
#include <limits>

namespace Nirvana {
namespace Core {
namespace Port {

struct Timer::Pool
{
	void initialize () noexcept
	{
		assert (!initialized);

		InitializeThreadpoolEnvironment (&callback_environ);
		thread_pool = CreateThreadpool (nullptr);
		assert (thread_pool);

		TP_POOL_STACK_INFORMATION si {
			Windows::NEUTRAL_FIBER_STACK_RESERVE,
			Windows::NEUTRAL_FIBER_STACK_COMMIT
		};

		NIRVANA_VERIFY (SetThreadpoolStackInformation (thread_pool, &si));
		SetThreadpoolThreadMaximum (thread_pool, Port::SystemInfo::hardware_concurrency ());
		NIRVANA_VERIFY (SetThreadpoolThreadMinimum (thread_pool, 0));

		SetThreadpoolCallbackPool (&callback_environ, thread_pool);

		cleanup_group = CreateThreadpoolCleanupGroup ();
		assert (cleanup_group);
		SetThreadpoolCallbackCleanupGroup (&callback_environ, cleanup_group, nullptr);

		initialized = true;
	}

	void terminate () noexcept
	{
		assert (initialized);

		initialized = false;

		CloseThreadpoolCleanupGroupMembers (cleanup_group, true, nullptr);
		CloseThreadpoolCleanupGroup (cleanup_group);
		CloseThreadpool (thread_pool);
		DestroyThreadpoolEnvironment (&callback_environ);
	}

	TP_TIMER* timer_create (Port::Timer& timer)
	{
		if (!initialized)
			throw_INITIALIZE ();
		TP_TIMER* p = CreateThreadpoolTimer (&timer_callback, &timer, &callback_environ);
		if (!p)
			throw_NO_MEMORY ();
		return p;
	}

	void timer_close (TP_TIMER* timer) noexcept
	{
		if (initialized) {
			SetThreadpoolTimer (timer, nullptr, 0, 0);
			WaitForThreadpoolTimerCallbacks (timer, true);
			CloseThreadpoolTimer (timer);
		}
	}

	static void CALLBACK timer_callback (TP_CALLBACK_INSTANCE*, void* context, TP_TIMER*);

	TP_CALLBACK_ENVIRON callback_environ;
	TP_POOL* thread_pool;
	TP_CLEANUP_GROUP* cleanup_group;
	volatile bool initialized;
};

void CALLBACK Timer::Pool::timer_callback (TP_CALLBACK_INSTANCE*, void* context, TP_TIMER*)
{
	((Timer*)context)->signal ();
}

Timer::Pool Timer::pool_ {};

void Timer::initialize () noexcept
{
	pool_.initialize ();
}

void Timer::terminate () noexcept
{
	pool_.terminate ();
}

bool Timer::initialized () noexcept
{
	return pool_.initialized;
}

Timer::Timer () :
	timer_ (pool_.timer_create (*this))
{}

Timer::~Timer ()
{
	pool_.timer_close (timer_);
}

void Timer::set (unsigned flags, TimeBase::TimeT due_time, TimeBase::TimeT period)
{
	if (!pool_.initialized)
		return;
	period /= TimeBase::MILLISECOND;
	if (period > (TimeBase::TimeT)std::numeric_limits <DWORD>::max ())
		throw_BAD_PARAM ();
	LARGE_INTEGER dt;
	if (flags & TIMER_ABSOLUTE)
		dt.QuadPart = due_time - Windows::WIN_TIME_OFFSET_SEC * TimeBase::SECOND;
	else {
		if (due_time > (TimeBase::TimeT)std::numeric_limits <LONGLONG>::max ())
			throw_BAD_PARAM ();
		dt.QuadPart = -(LONGLONG)due_time;
	}
	SetThreadpoolTimer (timer_, (FILETIME*)&dt, (DWORD)period, 0);
}

void Timer::cancel () noexcept
{
	if (pool_.initialized)
		SetThreadpoolTimer (timer_, nullptr, 0, 0);
}

}
}
}
