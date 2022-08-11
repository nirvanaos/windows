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
#include <ORB/OtherDomain.h>
#include "Mailslot.h"
#include "MailslotName.h"

namespace Nirvana {

using namespace Core::Windows;

namespace ESIOP {

void send_error_message (ProtDomainId domain_id, const void* msg, size_t size) NIRVANA_NOEXCEPT
{
	try {
		Mailslot ms;
		ms.open (MailslotName (domain_id));
		ms.send (msg, (DWORD)size);
	} catch (...) {
	}
}

OtherDomain* OtherDomain::create (ProtDomainId domain_id)
{
	throw_NO_IMPLEMENT ();
}

}
}
