/// \file
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
#ifndef NIRVANA_CORE_WINDOWS_TIMER_H_
#define NIRVANA_CORE_WINDOWS_TIMER_H_
#pragma once

#include "win32.h"
#include "../Port/Thread.h"
#include "error2errno.h"
#include <AtomicCounter.h>
#include <StaticallyAllocated.h>
#include <ObjectPool.h>

namespace Nirvana {
namespace Core {

class Timer;

namespace Windows {

class Timer : public StackElem,
	private Port::Thread
{
	static const unsigned MAX_POOL_SIZE = 10;

public:
	Timer ()
	{
		if (!(handles_ [HANDLE_TIMER] = CreateWaitableTimerW (nullptr, false, nullptr)))
			Windows::throw_last_error ();
		if (!(handles_ [HANDLE_DESTRUCT] = CreateEventW (nullptr, false, false, nullptr)))
			Windows::throw_last_error ();
		handles_ [HANDLE_TERMINATE] = global_->terminate_event ();
	}

	~Timer ();

	static void initialize ();
	static void terminate () noexcept;

	void connect (Core::Timer& timer) noexcept
	{
		timer_ = &timer;
	}

	void return_to_pool ()
	{

	}

private:
	friend class Thread;
	static unsigned long __stdcall thread_proc (Timer* _this);

private:
	class Global
	{
	public:
		Global () :
			pool_ (MAX_POOL_SIZE),
			terminate_ (CreateEventW (nullptr, true, false, nullptr)),
			destructed_ (CreateEventW (nullptr, true, true, nullptr)),
			timer_count_ (0)
		{}

		~Global ()
		{
			SetEvent (terminate_);
			WaitForSingleObject (destructed_, INFINITE);
		}

		HANDLE terminate_event () const noexcept
		{
			return terminate_;
		}

		void on_timer_construct () noexcept
		{
			if (1 == timer_count_.increment_seq ())
				ResetEvent (destructed_);
		}

		void on_timer_destruct () noexcept
		{
			if (0 == timer_count_.decrement_seq ())
				SetEvent (destructed_);
		}

		Timer& create ()
		{
			return pool_.create ();
		}

		void return_to_pool (Timer& timer) noexcept
		{
			pool_.release (timer);
		}

	private:
		ObjectPool <Timer> pool_;
		Windows::Handle terminate_;
		Windows::Handle destructed_;
		AtomicCounter <false> timer_count_;
	};

	static StaticallyAllocated <Global> global_;

	enum Handle
	{
		HANDLE_TIMER,
		HANDLE_DESTRUCT,
		HANDLE_TERMINATE,

		HANDLE_CNT
	};

	HANDLE handles_ [HANDLE_CNT];

	Core::Timer* timer_;
};

}
}
}

#endif
