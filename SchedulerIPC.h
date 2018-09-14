// Nirvana project
// Windows implementation.
// Scheduler IPC.

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERIPC_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERIPC_H_

#include <Nirvana.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerIPC
{
protected:
	struct ProcessStart
	{
		DWORD process_id;
	};

	struct ProcessStop
	{
		uint64_t process;
	};

	struct Ready
	{
		uint64_t process;
		uint64_t runnable;
		DeadlineTime deadline;
	};

	struct SchedulerMessage
	{
		enum int64_t
		{
			CORE_FREE,
			READY,
			UPDATE,
			PROCESS_START,
			PROCESS_STOP
		} tag;
		union
		{
			ProcessStart process_start;
			ProcessStop process_stop;
			Ready ready;
		} msg;
	};

	struct ProcessStartAck
	{
		uint64_t process;
	};

	struct Run
	{
		uint64_t runnable;
	};

	static WCHAR scheduler_mailslot_name_ [];
};

}
}
}

#endif
