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

#include <ORB/ESIOP.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

struct Message
{
	enum Type
	{
		PROCESS_START = (uint16_t)ESIOP::MessageType::MESSAGES_CNT,
		PROCESS_START_RESPONSE,
		PROCESS_STOP,
		SYSTEM_ERROR,
		SHUTDOWN
	};

	typedef ESIOP::MessageHeader Header;

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
		ESIOP::MessageBuffer esiop;
		ProcessStart process_start;
		ProcessStop process_stop;
		SystemError system_error;
		Shutdown shutdown;
	};
};

}
}
}

#endif
