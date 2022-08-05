/// \file
/// ESIOP definitions
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
namespace ESIOP {

/// Unique protection domain id.
typedef uint32_t ProtDomainId;

/// \returns Current protection domain id.
ProtDomainId current_domain_id () NIRVANA_NOEXCEPT;

/// SharedMemPtr points to the message recipient protection domain memory.
/// Sender allocates the message data via OtherMemory interface.
/// SharedMemPtr size is enough to store memory address
/// for any supported platform.
typedef uint64_t SharedMemPtr;

/// If an incoming request can't be processed, the system uses this function
/// to send ESIOP error message to caller.
/// This function must not throw exceptions.
/// 
/// \param domain_id The id of the error message recipient.
/// \param msg The error message buffer.
/// \param size The error message size.
void send_error_message (ProtDomainId domain_id, const void* msg, size_t size) NIRVANA_NOEXCEPT;

}
}

#endif
