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

protected:
	struct SyncDomainAddress
	{
		uint64_t mailslot_handle_;
		uint64_t sync_domain_ptr_;
	};

	struct MsgReady
	{
		SyncDomainAddress sync_domain;
		DeadlineTime deadline;
		BOOL update;
	};

	struct MsgRun
	{
		SyncDomainAddress sync_domain;
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
