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
#include "../AtomicCounter.h"
#include <Nirvana/bitutils.h>

namespace Nirvana {
namespace Core {
namespace Windows {

struct SchedulerItem
{
	uint64_t executor;
	SysDomain::ProtDomainInfo* protection_domain;

	SchedulerItem ()
	{}

	SchedulerItem (uint64_t prot_domain, uint64_t executor) :
		protection_domain (reinterpret_cast <SysDomain::ProtDomainInfo*> (prot_domain))
	{
		this->executor = executor;
	}

	SchedulerItem (::CORBA::Nirvana::Bridge <Executor>* executor) :
		protection_domain (nullptr)
	{
		this->executor = (uint64_t)executor;
	}

	bool operator < (const SchedulerItem& rhs) const
	{
		// Compare executors first to avoid grouping of the processes.
		if (executor < rhs.executor)
			return true;
		else if (executor > rhs.executor)
			return false;
		return protection_domain < protection_domain;
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
		Base (thread_count ()),
		in_proc_execute_ (thread_count ())
	{}

	/// Called by SchedulerImpl.
	void execute (const SchedulerItem& item, DeadlineTime deadline)
	{
		Execute msg;
		msg.executor = item.executor;
		msg.deadline = deadline;
		if (item.protection_domain)
			item.protection_domain->execute (msg);
		else
			in_proc_execute_.execute (msg);
	}

	// Implementation of Scheduler interface.

	static void _schedule (::CORBA::Nirvana::Bridge <Scheduler>* bridge, 
												 DeadlineTime deadline, ::CORBA::Nirvana::Bridge <Executor>* executor,
												 DeadlineTime deadline_prev, ::CORBA::Nirvana::EnvironmentBridge*);

	static void _core_free (::CORBA::Nirvana::Bridge <Scheduler>* bridge, ::CORBA::Nirvana::EnvironmentBridge*);

	/// Process mailslot message.
	void received (void* data, DWORD size);

private:
	/// Helper class for executing in the current process.
	class InProcExecute :
		public CompletionPortReceiver
	{
	public:
		InProcExecute (unsigned thread_count) :
			buffer_idx_ (-1)
		{
			AtomicCounter::UIntType buf_cnt = clp2 ((AtomicCounter::UIntType)thread_count);
			buffer_ = (Execute*)g_core_heap->allocate (nullptr, sizeof (Execute) * buf_cnt, 0);
			mask_ = buf_cnt - 1;
		}

		~InProcExecute ()
		{
			g_core_heap->release (buffer_, sizeof (Execute) * (mask_ + 1));
		}

		void execute (const Execute& msg)
		{
			Execute* buffer = buffer_ + (buffer_idx_.increment () & mask_);
			*buffer = msg;
			WorkerThreads::singleton ().post (*this, reinterpret_cast <OVERLAPPED*> (buffer), 0);
		}

	private:
		virtual void received (OVERLAPPED* ovl, DWORD size);

		Execute* buffer_;
		AtomicCounter buffer_idx_;
		AtomicCounter::UIntType mask_;
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
			Base::schedule (msg->msg.schedule.deadline, 
											SchedulerItem (msg->msg.schedule.protection_domain, msg->msg.schedule.executor),
											msg->msg.schedule.deadline_prev);
		}
		break;

	case SchedulerMessage::PROCESS_START:
		reinterpret_cast <SysDomain::ProtDomainInfo*> (msg->msg.process_start.protection_domain)->process_start (msg->msg.process_start.process_id);
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
