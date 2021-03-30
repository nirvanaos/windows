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
#include "shutdown.h"
#include "Message.h"
#include "Mailslot.h"
#include "MailslotName.h"

namespace Nirvana {
namespace Core {
namespace Windows {

bool shutdown ()
{
	Mailslot ms;
	if (ms.open (MailslotName (0))) {
		try {
			ms.send (Message::Shutdown ());
			return true;
		} catch (...) {
		}
	}
	return false;
}

}
}
}
