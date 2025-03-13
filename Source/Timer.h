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

namespace Port {

class Timer;

}

namespace Windows {

class Timer : public StackElem,
	private Port::Thread
{
	friend class ObjectCreator <Timer*>;
	using Thread = Port::Thread;

public:
	static void initialize ();
	static void terminate () noexcept;

	static Timer& create (Port::Timer& facade)
	{
		Timer& t = global_->create ();
		t.facade_ = &facade;
	}

	void release () noexcept
	{
#ifndef NDEBUG
		facade_ = nullptr;
#endif
		global_->release (*this);
	}

	void set (const LARGE_INTEGER& due_time, LONG period, bool resume);
	void cancel () noexcept;

private:
	Timer ()
#ifndef NDEBUG
		: facade_ (nullptr)
#endif
	{
		if (!(handles_ [HANDLE_TIMER] = CreateWaitableTimerW (nullptr, false, nullptr)))
			Windows::throw_last_error ();
		if (!(handles_ [HANDLE_DESTRUCT] = CreateEventW (nullptr, false, false, nullptr)))
			Windows::throw_last_error ();
		handles_ [HANDLE_TERMINATE] = global_->terminate_event ();

		Thread::create (this, Windows::TIMER_THREAD_PRIORITY);

		global_->on_timer_construct ();
	}

	~Timer ();

private:
	friend class Thread;
	static unsigned long __stdcall thread_proc (Timer* _this);

private:
	enum Handle
	{
		HANDLE_TIMER,
		HANDLE_DESTRUCT,
		HANDLE_TERMINATE,

		HANDLE_CNT
	};

	HANDLE handles_ [HANDLE_CNT];

	Port::Timer* facade_;

	class Global
	{
	public:
		Global () :
			terminate_ (CreateEventW (nullptr, true, false, nullptr)),
			destructed_ (CreateEventW (nullptr, true, true, nullptr)),
			timer_count_ (0)
		{}

		~Global ()
		{
			SetEvent (terminate_);
			WaitForSingleObject (destructed_, INFINITE);
			CloseHandle (terminate_);
			CloseHandle (destructed_);
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
			return *pool_.create ();
		}

		void release (Timer& timer) noexcept
		{
			pool_.release (&timer);
		}

	private:
		ObjectPool <Timer*> pool_;
		HANDLE terminate_;
		HANDLE destructed_;
		AtomicCounter <false> timer_count_;
	};

	static StaticallyAllocated <Global> global_;
};

}
}
}

#endif
