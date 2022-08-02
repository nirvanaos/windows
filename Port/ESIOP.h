/// \file
/// ESIOP portability layer
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
#ifndef NIRVANA_CORE_PORT_ESIOP_H_
#define NIRVANA_CORE_PORT_ESIOP_H_
#pragma once

#include <Nirvana/NirvanaBase.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// Unique protection domain id
typedef uint32_t ProtDomainId;

/// Protection domain handle
struct ProtDomainHandle
{
	/// ProtDomainHandle must be convertible into ProtDomainId.
	/// Or even may be the same type.
	operator ProtDomainId () const
	{
		return process_id;
	}

	ProtDomainId process_id;
	uint32_t mailslot_handle;
	uint32_t memory_handle;
};

/// MaxPlatformPtr points to the message recipient protection domain memory.
/// Sender allocates the message data via OtherMemory interface.
/// Core::Port::MaxPlatformPtr size is enough to store memory address
/// for any supported platform.
typedef uint64_t MaxPlatformPtr;

/// User security token.
typedef uint32_t UserToken;

}
}
}

#endif
