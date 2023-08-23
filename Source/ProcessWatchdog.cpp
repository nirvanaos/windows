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

/*
#include "ProcessWatchdog.h"

namespace Nirvana {
namespace Core {
namespace Windows {

inline void ProcessWatchdog::process_start (ProcessStartMessage& msg)
{
	HANDLE process = OpenProcess (SYNCHRONIZE, FALSE, msg.process_id);
	if (process) {
		try {
			semaphore_handles_.push_back ((HANDLE)(uintptr_t)msg.executor_id);
			wait_handles_.push_back (process);
		} catch (...) {
			scheduler_.on_error (CORBA::SystemException::EC_NO_MEMORY);
		}
	}
}

inline void ProcessWatchdog::process_stop (size_t idx)
{
	HANDLE sem0 = semaphore_handles_ [idx];
	// Duplicate and close semaphore to cause Scheduler failure on ReleaseSemaphore.
	HANDLE sem1;
	HANDLE proc = GetCurrentProcess ();
	verify (DuplicateHandle (proc, sem0, proc, &sem1, 0, FALSE, DUPLICATE_SAME_ACCESS));
	verify (CloseHandle (sem0));
	// Release all processor cores owned by the dead process.
	while (WAIT_OBJECT_0 == WaitForSingleObject (sem1, 0)) {
		scheduler_.core_free ();
	}
	verify (CloseHandle (sem1));
	verify (CloseHandle (wait_handles_ [idx + 2]));
	semaphore_handles_.erase (semaphore_handles_.begin () + idx);
	wait_handles_.erase (wait_handles_.begin () + idx + 2);
}

inline void ProcessWatchdog::thread_proc ()
{
	OVERLAPPED ovl;
	zero (ovl);
	ovl.hEvent = wait_handles_ [MAILSLOT_EVENT];
	ProcessStartMessage msg;
	DWORD cb_read;
	ReadFile (mailslot_, &msg, sizeof (msg), &cb_read, &ovl);
	for (;;) {
		DWORD w = WaitForMultipleObjects ((DWORD)wait_handles_.size (), wait_handles_.data (), FALSE, INFINITE);
		if (WAIT_OBJECT_0 <= w && w < wait_handles_.size ()) {
			w -= WAIT_OBJECT_0;
			switch (w) {
				case MAILSLOT_EVENT:
					if (GetOverlappedResult (mailslot_, &ovl, &cb_read, FALSE) && sizeof (ProcessStartMessage) == cb_read)
						process_start (msg);
					ReadFile (mailslot_, &msg, sizeof (msg), &cb_read, &ovl);
					break;

				case TERMINATE_EVENT:
					CancelIo (mailslot_);
					return;

				default:
					process_stop (w - 2);
			}
		}
	}
}

DWORD WINAPI ProcessWatchdog::s_thread_proc (void* _this)
{
	((ProcessWatchdog*)_this)->thread_proc ();
	return 0;
}

}
}
}
*/
