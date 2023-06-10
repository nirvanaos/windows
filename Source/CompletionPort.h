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
#ifndef NIRVANA_CORE_WINDOWS_COMPLETIONPORT_H_
#define NIRVANA_CORE_WINDOWS_COMPLETIONPORT_H_
#pragma once

#include <assert.h>
#include "CompletionPortReceiver.h"
#include "../Port/SystemInfo.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

/// Windows I/O completion port wrapper.
class CompletionPort
{
private:
	CompletionPort (const CompletionPort&) = delete;
	CompletionPort& operator = (const CompletionPort&) = delete;

public:
	static unsigned int thread_count ()
	{
		return Port::SystemInfo::hardware_concurrency ();
	}

	CompletionPort ();

	~CompletionPort ()
	{
		if (completion_port_)
			CloseHandle (completion_port_);
	}

	/// Add a CompletionPortReceiver object listening this completion port.
	void add_receiver (HANDLE hf, CompletionPortReceiver& receiver)
	{
		create (hf, &receiver);
	}

	/// Post an I/O completion packet to an I/O completion port.
	void post (CompletionPortReceiver& receiver, OVERLAPPED* param, DWORD size) noexcept
	{
		verify (PostQueuedCompletionStatus (completion_port_, size, (ULONG_PTR)&receiver, param));
	}

	/// On close completion port all threads will return with `ERROR_ABANDONED_WAIT_0` error code.
	void terminate () noexcept
	{
		HANDLE port = completion_port_;
		completion_port_ = nullptr;
		if (port)
			CloseHandle (port);
	}

	/// Worker thread procedure.
	void thread_proc ();

protected:
	/// Ensure that port exists.
	void start ()
	{
		if (!completion_port_)
			create (INVALID_HANDLE_VALUE, nullptr);
	}

private:
	void create (HANDLE hfile, CompletionPortReceiver* receiver);

private:
	HANDLE completion_port_;
};

}
}
}

#endif
