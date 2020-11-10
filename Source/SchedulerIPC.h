// Nirvana project
// Windows implementation.
// Scheduler IPC.

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERIPC_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERIPC_H_

#define OBJ_NAME_PREFIX L"Nirvana"

#define SCHEDULER_MAILSLOT_NAME L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\scheduler_mailslot"

namespace Nirvana {
namespace Core {
namespace Windows {

struct SchedulerMessage
{
	/// A new process (protection domain) is started.
	/// 
	struct ProcessStart
	{
		uint32_t process_id;

		/// Handle of the semaphore.
		/// SysDomain asynchronously calls DuplicateHandle and sends resulting
		/// handle to protection domain mailslot as "executor_id".
		/// SchedulerSlave uses it to identify itself in the Schedule message.
		uint32_t semaphore_handle : 31,
			x64 : 1;
	};

	struct Schedule
	{
		uint32_t executor_id;
		DeadlineTime deadline;
	};

	struct Reschedule
	{
		uint32_t protection_domain;
		DeadlineTime deadline;
		DeadlineTime deadline_prev;
	};

	struct CoreFree
	{
		uint32_t unused;
	};

	union Buffer
	{
		CoreFree core_free;
		Schedule schedule;
		Reschedule reschedule;
		ProcessStart process_start;
	};
};

struct MsgFromScheduler
{
	struct ProcessStart
	{
		int32_t error;
		Port::ObjectAddress system_domain;
	};

	struct Execute
	{};

	enum int64_t
	{
		PROCESS_START,
		SCHEDULE,
		PROCESS_START,
		PROCESS_STOP
	}
	tag;

	union
	{
		ProcessStart process_start;
		Execute execute;
	}
	msg;
};

}
}
}

#endif
