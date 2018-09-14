// Nirvana project
// Windows implementation.
// SchedulerWindows class. 

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERWINDOWS_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERWINDOWS_H_

#include "SchedulerIPC.h"
#include "PostOffice.h"
#include "ThreadPoolable.h"
#include "../../Interface/Scheduler.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerWindows : 
	public SchedulerIPC,
	public PostOffice <SchedulerWindows, sizeof (SchedulerMessage), ThreadPoolable, THREAD_PRIORITY_TIME_CRITICAL>,
	public ::CORBA::Nirvana::Servant <SchedulerWindows, Core::Scheduler>
{
public:
	struct ProcessInfo
	{
		HANDLE mailslot;
	};

	struct QueueItem
	{
		ProcessInfo* process;
		uint64_t runnable;
	};

	void run (QueueItem)
	{}

	virtual void received (OVERLAPPED* ovl, DWORD size);

	void schedule (DeadlineTime, Runnable_ptr);
	void back_off (::CORBA::ULong);
};

}
}
}

#endif
