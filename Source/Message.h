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
#ifndef NIRVANA_CORE_WINDOWS_MESSAGE_H_
#define NIRVANA_CORE_WINDOWS_MESSAGE_H_
#pragma once

#include <ORB/Message.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

#pragma pack(push, 1)

struct Message : ESIOP::Message
{
	enum class Type : uint16_t
	{
		PROCESS_START = (uint16_t)ESIOP::Message::Type::RESERVED_MESSAGES,
		PROCESS_START_RESPONSE,
		PROCESS_STOP,
		SYSTEM_ERROR,
		SHUTDOWN
	};

	struct Header
	{
		Type type;
		uint16_t flags; // Reserved, zero

		Header ()
		{}

		Header (Type t) :
			type (t),
			flags (0)
		{}
	};

	/// A new process (protection domain) is started.
	struct ProcessStart : Header
	{
		uint32_t process_id;

		ProcessStart (uint32_t id) :
			Header (Type::PROCESS_START),
			process_id (id)
		{}
	};

	struct ProcessStartResponse : Header
	{
		uint32_t sys_process_id;

		/// Handle of the semaphore.
		uint32_t executor_id;

		ProcessStartResponse ()
		{}

		ProcessStartResponse (uint32_t process_id, void* semaphore) :
			Header (Type::PROCESS_START_RESPONSE),
			sys_process_id (process_id),
			executor_id ((uint32_t)(uintptr_t)semaphore)
		{}
	};

	struct ProcessStop : Header
	{
		uint32_t process_id;

		ProcessStop (uint32_t id) :
			Header (Type::PROCESS_STOP),
			process_id (id)
		{}
	};

	struct SystemError : Header
	{
		int32_t error_code;

		SystemError (int32_t err) :
			Header (Type::SYSTEM_ERROR),
			error_code (err)
		{}
	};

	struct Shutdown : Header
	{
		Shutdown () :
			Header (Type::SHUTDOWN)
		{}
	};

	union Buffer
	{
		ESIOP::Message::Buffer esiop;
		ProcessStart process_start;
		ProcessStop process_stop;
		SystemError system_error;
		Shutdown shutdown;
	};
};

#pragma pack(pop)

}
}
}

#endif
