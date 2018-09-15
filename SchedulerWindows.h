// Nirvana project
// Windows implementation.
// SchedulerWindows class. 

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERWINDOWS_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERWINDOWS_H_

#include "SchedulerIPC.h"
#include "PostOffice.h"
#include "Mailslot.h"
#include "ThreadPoolable.h"
#include "SchedulerServant.h"
#include "WorkerThreads.h"
#include "../SysDomain.h"
#include "../SchedulerImpl.h"

namespace Nirvana {
namespace Core {
namespace Windows {

struct SchedulerItem
{
	SysDomain::ProtDomainInfo* process;
	uint64_t runnable;

	bool operator < (const SchedulerItem& rhs) const
	{
		// Compare runnable first to avoid grouping of the processes.
		if (runnable < rhs.runnable)
			return true;
		else if (runnable > rhs.runnable)
			return false;
		return process < process;
	}
};

class SchedulerWindows :
	public SchedulerServant <SchedulerWindows>,
	public SchedulerIPC,
	public PostOffice <SchedulerWindows, sizeof (SchedulerIPC::SchedulerMessage), ThreadPoolable, THREAD_PRIORITY_TIME_CRITICAL>,
	private SchedulerImpl <SchedulerWindows, SchedulerItem>
{
public:
	typedef SchedulerImpl <SchedulerWindows, SchedulerItem> Base;

	SchedulerWindows () :
		Base (thread_count ())
	{}

	/// Called by SchedulerImpl.
	void execute (const SchedulerItem& item)
	{
		if (item.process)
			item.process->execute (item.runnable);
		else
			in_proc_execute_.execute (item.runnable);
	}

	// Implementation of Scheduler interface.

	static void _schedule (::CORBA::Nirvana::Bridge <Scheduler>* bridge, 
												 DeadlineTime deadline, ::CORBA::Nirvana::Bridge <Runnable>* runnable,
												 DeadlineTime deadline_prev, ::CORBA::Nirvana::EnvironmentBridge*);

	static void _core_free (::CORBA::Nirvana::Bridge <Scheduler>* bridge, ::CORBA::Nirvana::EnvironmentBridge*);

	/// Process mailslot message.
	void received (void* data, DWORD size);

private:
	class InProcExecute :
		public CompletionPortReceiver
	{
	public:
		void execute (uint64_t runnable)
		{
			WorkerThreads::singleton ().post (*this, reinterpret_cast <OVERLAPPED*> (runnable), 0);
		}
	private:
		virtual void received (OVERLAPPED* ovl, DWORD size);
	} 
	in_proc_execute_;
};

inline
void SchedulerWindows::received (void* data, DWORD size)
{
	SchedulerMessage* msg = (SchedulerMessage*)data;

	switch (msg->tag) {
	case SchedulerMessage::CORE_FREE:
		Base::core_free ();
		break;

	case SchedulerMessage::SCHEDULE:
		{
			SchedulerItem item;
			item.process = (SysDomain::ProtDomainInfo*)msg->msg.schedule.process;
			item.runnable = msg->msg.schedule.runnable;
			Base::schedule (msg->msg.schedule.deadline, item, msg->msg.schedule.deadline_prev);
		}
		break;

	case SchedulerMessage::PROCESS_START:
		((SysDomain::ProtDomainInfo*)msg->msg.process_start.process)->process_start (msg->msg.process_start.process_id);
		break;

		//case SchedulerMessage::PROCESS_STOP:

	default:
		assert (false);
	}
}

}
}
}

#endif
