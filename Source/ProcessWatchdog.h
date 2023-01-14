/// \file
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
#ifndef NIRVANA_CORE_WINDOWS_PROCESSWATCHDOG_H_
#define NIRVANA_CORE_WINDOWS_PROCESSWATCHDOG_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include "SchedulerMessage.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerMaster;

class ProcessWatchdog
{
public:
	ProcessWatchdog (SchedulerMaster& scheduler) :
		scheduler_ (scheduler),
		mailslot_ (INVALID_HANDLE_VALUE),
		thread_ (nullptr),
		terminate_event_ (nullptr)
	{}

	~ProcessWatchdog ()
	{}

	bool start ()
	{
		mailslot_ = CreateMailslotW (WATCHDOG_MAILSLOT_NAME, sizeof (ProcessStartMessage), MAILSLOT_WAIT_FOREVER, nullptr);
		if (INVALID_HANDLE_VALUE == mailslot_)
			return false;
		semaphore_handles_.reserve (16 - 2);
		wait_handles_.reserve (16);
		wait_handles_.push_back (CreateEventW (nullptr, TRUE, FALSE, nullptr));
		wait_handles_.push_back (CreateEventW (nullptr, TRUE, FALSE, nullptr));
		terminate_event_ = wait_handles_.back ();
		thread_ = CreateThread (nullptr, 0x10000, s_thread_proc, this, 0, nullptr);
		return true;
	}

	void terminate ()
	{
		if (terminate_event_) {
			SetEvent (terminate_event_);
			terminate_event_ = nullptr;
			WaitForSingleObject (thread_, INFINITE);
			CloseHandle (thread_);
		}
		for (HANDLE h : wait_handles_)
			CloseHandle (h);
		for (HANDLE h : semaphore_handles_)
			CloseHandle (h);
		CloseHandle (mailslot_);
	}

private:
	static DWORD WINAPI s_thread_proc (void* _this);
	void thread_proc ();
	void process_start (ProcessStartMessage& msg);
	void process_stop (size_t idx);

private:
	static const unsigned MAILSLOT_EVENT = 0;
	static const unsigned TERMINATE_EVENT = 1;

	SchedulerMaster& scheduler_;
	HANDLE mailslot_;
	HANDLE thread_;
	HANDLE terminate_event_;
	std::vector <HANDLE, SharedAllocator <HANDLE> > wait_handles_;
	std::vector <HANDLE, SharedAllocator <HANDLE> > semaphore_handles_;
};

}
}
}

#endif
