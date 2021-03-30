/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
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
#include "CompletionPort.h"
#include <Nirvana/throw_exception.h>

namespace Nirvana {
namespace Core {
namespace Windows {

CompletionPort::CompletionPort () :
	completion_port_ (nullptr)
{}

void CompletionPort::create (HANDLE hfile, CompletionPortReceiver* receiver)
{
	HANDLE port = CreateIoCompletionPort (hfile, completion_port_, (ULONG_PTR)receiver, thread_count ());
	if (!port)
		throw_INITIALIZE ();
	assert (completion_port_ == nullptr || completion_port_ == port);
	completion_port_ = port;
}

void CompletionPort::thread_proc () NIRVANA_NOEXCEPT
{
	while (completion_port_) {
		ULONG_PTR key;
		OVERLAPPED* ovl;
		DWORD size;
		if (GetQueuedCompletionStatus (completion_port_, &size, &key, &ovl, INFINITE))
			reinterpret_cast <CompletionPortReceiver*> (key)->received (ovl, size);
		else {
			DWORD err = GetLastError ();
			switch (err) {
			case ERROR_OPERATION_ABORTED:
			case ERROR_ABANDONED_WAIT_0:
			case ERROR_INVALID_HANDLE:
				break;

			default:
				assert (false);
			}
		}
	}
}

}
}
}
