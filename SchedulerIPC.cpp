// Nirvana project
// Windows implementation.
// Scheduler IPC.

#include "SchedulerIPC.h"
#include <ORB.h>

namespace Nirvana {
namespace Core {
namespace Windows {

WCHAR SchedulerIPC::scheduler_mailslot_name_ [] = L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\scheduler_mailslot";
WCHAR SchedulerIPC::free_cores_semaphore_name_ [] = OBJ_NAME_PREFIX L".free_cores_semaphore";

HANDLE SchedulerIPC::create_mailslot (LPCWSTR name, DWORD max_msg_size)
{
	HANDLE mailslot = CreateMailslotW (name, max_msg_size, MAILSLOT_WAIT_FOREVER, nullptr);
	if (INVALID_HANDLE_VALUE == mailslot)
		throw ::CORBA::INITIALIZE ();
	return mailslot;
}

HANDLE SchedulerIPC::open_mailslot (LPCWSTR name)
{
	HANDLE mailslot = CreateFileW (name, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 
																 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
	if (INVALID_HANDLE_VALUE == mailslot)
		throw ::CORBA::INITIALIZE ();
	return mailslot;
}

HANDLE SchedulerIPC::open_run_mailslot (DWORD process_id, bool create)
{
	static const WCHAR fmt [] = L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\run_mailslot%08X";
	WCHAR name [_countof (fmt) + 8 - 3];
	wsprintfW (name, fmt, process_id);

	HANDLE mailslot;
	if (create)
		mailslot = create_mailslot (name, sizeof (MsgRun));
	else
		mailslot = open_mailslot (name);
	return mailslot;
}

}
}
}