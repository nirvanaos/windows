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
public:
	static unsigned int hardware_concurrency ()
	{
		::SYSTEM_INFO si;
		::GetSystemInfo (&si);
		return si.dwNumberOfProcessors;
	}

protected:
	struct MsgProcessStart
	{
		DWORD process_id;
		DWORD reserved;
	};

	struct MsgSyncDomainReady
	{
		DeadlineTime deadline;
		ULONG_PTR sync_domain;
		DWORD process_id;
		BOOL update;
	};

	struct MsgProcessStop
	{
		DWORD process_id;
	};

	union SchedulerMessage
	{
		MsgSyncDomainReady sync_domain_ready;
		MsgProcessStart process_start;
		MsgProcessStop process_stop;
	};

	struct MsgRun
	{
		ULONG_PTR sync_domain;
	};

	static HANDLE create_mailslot (LPCWSTR name, DWORD max_msg_size);
	static HANDLE open_mailslot (LPCWSTR name);
	static HANDLE open_run_mailslot (DWORD process_id, bool create);
	
	template <class Msg>
	static BOOL send (HANDLE mailslot, const Msg& msg)
	{
		DWORD cb;
		return WriteFile (mailslot, &msg, sizeof (msg), &cb, nullptr);
	}

protected:
	static WCHAR scheduler_mailslot_name_ [];
	static WCHAR free_cores_semaphore_name_ [];
};

}
}
}

#endif
