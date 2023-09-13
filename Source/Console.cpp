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
#include "../Port/Console.h"
#include "error2errno.h"

namespace Nirvana {
namespace Core {

using namespace Windows;

namespace Port {

Console::Console () :
	FileAccessChar (nullptr),
	read_thread_ (nullptr),
	read_event_ (nullptr),
	read_stop_ (false)
{
	if (!IsDebuggerPresent ()) {
		if (!AttachConsole (ATTACH_PARENT_PROCESS))
			AllocConsole ();
	} else
		AllocConsole ();
	handle_out_ = GetStdHandle (STD_OUTPUT_HANDLE);
	handle_in_ = GetStdHandle (STD_INPUT_HANDLE);
}

Console::~Console ()
{
	if (read_event_) {
		if (read_thread_) {
			read_stop_ = true;
			SetEvent (read_event_);
			WaitForSingleObject (read_thread_, INFINITE);
			CloseHandle (read_thread_);
		}
		CloseHandle (read_event_);
	}
	FreeConsole ();
}

void Console::read_start () noexcept
{
	if (!read_event_) {
		read_event_ = CreateEventW (nullptr, true, false, nullptr);
		if (!read_event_)
			on_read_error (error2errno (GetLastError ()));
	}
	if (!read_thread_) {
		read_thread_ = CreateThread (nullptr, NEUTRAL_FIBER_STACK_RESERVE, s_read_proc, this,
			STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED, nullptr);
		if (!read_thread_)
			on_read_error (error2errno (GetLastError ()));
		else {
			SetThreadPriority (read_thread_, IO_THREAD_PRIORITY);
			ResumeThread (read_thread_);
		}
	}
}

void Console::read_cancel () noexcept
{
	CancelSynchronousIo (read_thread_);
}

inline
void Console::read_proc () noexcept
{
	for (;;) {
		WaitForSingleObject (read_event_, INFINITE);
		if (read_stop_)
			break;
		char c;
		DWORD cbr;
		if (ReadFile (handle_in_, &c, 1, &cbr, nullptr))
			on_read (c);
		else {
			DWORD err = GetLastError ();
			if (ERROR_OPERATION_ABORTED != err)
				on_read_error (error2errno (err));
		}
	}
}

unsigned long __stdcall Console::s_read_proc (void* p) noexcept
{
	((Console*)p)->read_proc ();
	return 0;
}

Ref <IO_Request> Console::write_start (const IDL::String& data)
{
	Ref <IO_Request> rq = Ref <IO_Request>::create <RequestWrite> ();
	DWORD cbw;
	IO_Result res (0, 0);
	if (!WriteFile (handle_out_, data.data (), (DWORD)data.size (), &cbw, nullptr))
		res.error = error2errno (GetLastError ());
	res.size = cbw;
	rq->signal (res);
	return rq;
}

}
}
}
