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
#ifndef NIRVANA_CORE_WINDOWS_MAILSLOTREADER_H_
#define NIRVANA_CORE_WINDOWS_MAILSLOTREADER_H_
#pragma once

#include <assert.h>
#include "BufferPool.h"
#include "CompletionPort.h"
#include <Nirvana/real_copy.h>

namespace Nirvana {

namespace Core {
namespace Windows {

/// Derived class must override `virtual void CompletionPortReceiver::received()` method to process data.
/// Overridden method must get data pointer by call data(ovl), read the data
/// and immediatelly call `MailslotReader::enqueue_buffer()` method.
class MailslotReader :
	public CompletionPortReceiver,
	public BufferPool
{
public:
	/// Create mailslot.
	/// \param mailslot_name Name of the mailslot for incoming messages.
	/// \returns `true` on success. `false` if mailslot already exists.
	/// \throws CORBA::INITIALIZE
	bool create_mailslot (LPCWSTR mailslot_name);

	HANDLE mailslot_handle () const
	{
		return handle_;
	}

protected:
	MailslotReader (size_t buffer_count, DWORD max_msg_size);
	~MailslotReader ();

	/// Put the reader to work.
	void start (CompletionPort& port);

	/// Stop all work.
	void terminate () noexcept
	{}

	/// Enqueue read request.
	void enqueue_buffer (OVERLAPPED* ovl) noexcept
	{
		zero (*ovl);
		if (!ReadFile (handle_, data (ovl), (DWORD)buffer_size (), nullptr, ovl))
			assert (ERROR_IO_PENDING == GetLastError ());
	}

private:
	HANDLE handle_;
	DWORD max_msg_size_;
};

}
}
}

#endif
