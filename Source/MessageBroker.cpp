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
#include "MessageBroker.h"
#include <Scheduler.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void MessageBroker::received (void* message, DWORD size) NIRVANA_NOEXCEPT
{
	const Message::Header* hdr = (const Message::Header*)message;
	if (hdr->message_type < ESIOP::MessageType::MESSAGES_CNT)
		ESIOP::dispatch_message (*hdr);
	else
		switch (hdr->message_type) {
			case Message::Type::SHUTDOWN:
				Scheduler::shutdown ();
				break;
		}
}

}
}
}
