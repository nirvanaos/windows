// Nirvana project
// Windows implementation.
// Scheduler IPC.

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERIPC_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERIPC_H_

#include <Nirvana.h>
#include "win32.h"

#define SCHEDULER_MAILSLOT_NAME L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\scheduler_mailslot"
#define EXECUTE_MAILSLOT_PREFIX L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\execute_mailslot"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerIPC
{
public:
	struct ProcessStart
	{
		uint64_t protection_domain;
		DWORD process_id;
	};

	struct ProcessStop
	{
		uint64_t protection_domain;
		DWORD process_id;
	};

	struct Schedule
	{
		uint64_t protection_domain;
		uint64_t executor;
		DeadlineTime deadline;
		DeadlineTime deadline_prev;
	};

	struct CoreFree
	{};

	struct SchedulerMessage
	{
		enum int64_t
		{
			CORE_FREE,
			SCHEDULE,
			PROCESS_START,
			PROCESS_STOP
		} tag;

		union
		{
			CoreFree core_free;
			Schedule schedule;
			ProcessStart process_start;
			ProcessStop process_stop;
		} msg;

		DWORD size () const
		{
			static const DWORD sizes [5] = {
				sizeof (tag) + sizeof (msg.core_free),
				sizeof (tag) + sizeof (msg.schedule),
				sizeof (tag) + sizeof (msg.process_start),
				sizeof (tag) + sizeof (msg.process_stop)
			};

			return sizes [tag];
		}
	};

	struct ProcessStartAck
	{
		uint64_t error;
	};

	struct Execute
	{
		uint64_t executor;
		DeadlineTime deadline;
	};
};

}
}
}

#endif
