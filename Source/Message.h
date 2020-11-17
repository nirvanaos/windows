#ifndef NIRVANA_CORE_WINDOWS_MESSAGE_H_
#define NIRVANA_CORE_WINDOWS_MESSAGE_H_

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
		SYSTEM_ERROR
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
			executor_id ((uint32_t)semaphore)
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

	union Buffer
	{
		ESIOP::Message::Buffer esiop;
		ProcessStart process_start;
		ProcessStop process_stop;
	};
};

#pragma pack(pop)

}
}
}

#endif
