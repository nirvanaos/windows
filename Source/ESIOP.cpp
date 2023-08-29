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
#include <ORB/ESIOP.h>
#include <Port/Security.h>
#include "Mailslot.h"
#include "object_name.h"
#include "win32.h"
#include "error2errno.h"

using namespace Nirvana::Core::Windows;

namespace ESIOP {

void send_error_message (ProtDomainId domain_id, const void* msg, size_t size) noexcept
{
	try {
		Mailslot ms;
		ms.open (object_name (MAILSLOT_PREFIX, domain_id));
		ms.send (msg, (DWORD)size);
	} catch (...) {
	}
}

void send_shutdown (ProtDomainId domain_id)
{
	HANDLE process = OpenProcess (PROCESS_DUP_HANDLE, false, domain_id);
	if (!process)
		throw_last_error ();

	Nirvana::Core::Port::Security::Context local_context = Nirvana::Core::Port::Security::get_domain_context ();

	HANDLE token2;
	if (!DuplicateHandle (GetCurrentProcess (), local_context, process, &token2, 0, FALSE,
		DUPLICATE_SAME_ACCESS))
		throw_last_error ();

	try {
		Shutdown msg ((Nirvana::Core::Port::Security::ContextABI)(uintptr_t)token2);
		Mailslot ms;
		ms.open (object_name (MAILSLOT_PREFIX, domain_id));
		ms.send (msg);
	} catch (...) {
		CloseHandle (token2);
		throw;
	}
}

}
