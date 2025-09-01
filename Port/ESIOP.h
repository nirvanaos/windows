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
#ifndef NIRVANA_ESIOP_PORT_H_
#define NIRVANA_ESIOP_PORT_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/platform.h>
#include "config.h"
#include <type_traits>

extern "C" __declspec (dllimport)
unsigned long __stdcall GetCurrentProcessId (void);

namespace Nirvana {
namespace Core {
namespace Windows {

extern uint32_t cur_process_id;
extern uint32_t sys_process_id;
extern bool is_system_domain;

}
}
}

namespace ESIOP {

/// If some of the supported platforms have different endianness, this constant must be `true`.
const bool PLATFORMS_ENDIAN_DIFFERENT = false;

/// Unique protection domain id.
typedef uint32_t ProtDomainId;

/// \returns Current protection domain id.
inline ProtDomainId current_domain_id (void) noexcept
{
	return Nirvana::Core::Windows::cur_process_id;
}

/// \returns System protection domain id.
inline ProtDomainId sys_domain_id (void) noexcept
{
	return Nirvana::Core::Windows::sys_process_id;
}

/// SharedMemPtr points to the message recipient protection domain memory.
/// Sender allocates the message data via OtherMemory interface.
/// SharedMemPtr size is enough to store memory address
/// for any supported platform.
using SharedMemPtr = std::conditional_t <
Nirvana::HOST_PLATFORM == Nirvana::PLATFORM_X86
, uint32_t, uint64_t>;

/// If an incoming request can't be processed, the system uses this function
/// to send ESIOP error message to caller.
/// This function must not throw exceptions.
/// 
/// \param domain_id The id of the error message recipient.
/// \param msg The error message buffer.
/// \param size The error message size.
void send_error_message (ProtDomainId domain_id, const void* msg, size_t size) noexcept;

void send_shutdown (ProtDomainId domain_id, unsigned flags);

}

#endif
