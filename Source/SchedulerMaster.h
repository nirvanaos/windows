// Nirvana project
// Windows implementation.
// SchedulerMaster class. 

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERMASTER_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERMASTER_H_

#include "SchedulerBase.h"
#include <SchedulerImpl.h>
#include "PostOffice.h"
#include "WorkerThreads.h"
#include "SchedulerMessage.h"
#include "MessageBroker.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerItem
{
public:
	SchedulerItem ()
	{}

	SchedulerItem (uint32_t executor_id) :
		executor_ (executor_id | IS_SEMAPHORE)
	{}

	SchedulerItem (Executor& executor) :
		executor_ ((uintptr_t)&executor)
	{}

	uintptr_t is_semaphore () const
	{
		return executor_ & IS_SEMAPHORE;
	}

	HANDLE semaphore () const
	{
		assert (is_semaphore ());
		return (HANDLE)(executor_ & ~IS_SEMAPHORE);
	}

	Executor& executor () const
	{
		assert (!is_semaphore ());
		return *(Executor*)executor_;
	}

	bool operator < (const SchedulerItem& rhs) const
	{
		// Each item must be unique even if executors are the same.
		return this < &rhs;
	}

private:
	static const uintptr_t IS_SEMAPHORE = ~((~(uintptr_t)0) >> 1);
	uintptr_t executor_;
};

class SchedulerMaster :
	public SchedulerBase <SchedulerMaster>,
	public PostOffice <SchedulerMaster, sizeof (SchedulerMessage::Buffer), SCHEDULER_THREAD_PRIORITY>,
	public SchedulerImpl <SchedulerMaster, SchedulerItem>
{
	typedef SchedulerImpl <SchedulerMaster, SchedulerItem> Base;
	typedef PostOffice <SchedulerMaster, sizeof (SchedulerMessage::Buffer), SCHEDULER_THREAD_PRIORITY> Office;
public:
	SchedulerMaster () :
		error_ (0)
	{}

	/// Main loop.
	/// \param startup System domain startup runnable object.
	/// \param deadline Startup deadline.
	/// \returns `false` if system domain is already running.
	bool run (Runnable& startup, DeadlineTime deadline);

	// Implementation of SchedulerAbstract.
	virtual void create_item ();
	virtual void delete_item () NIRVANA_NOEXCEPT;
	virtual void schedule (DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT;
	virtual bool reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) NIRVANA_NOEXCEPT;

	/// Process mailslot message.
	void received (void* data, DWORD size);

	/// Called by SchedulerImpl
	void execute (SchedulerItem& item) NIRVANA_NOEXCEPT
	{
		if (item.is_semaphore ())
			ReleaseSemaphore (item.semaphore (), 1, nullptr);
		else
			worker_threads_.execute (item.executor ());
	}

private:
	/// Helper class for executing in the current process.
	class WorkerThreads :
		public Windows::WorkerThreads <CompletionPort>,
		public CompletionPortReceiver
	{
	public:
		void execute (Executor& executor) NIRVANA_NOEXCEPT
		{
			post (*this, reinterpret_cast <OVERLAPPED*> (&executor), 0);
		}

	private:
		virtual void received (OVERLAPPED* ovl, DWORD size);

	}
	worker_threads_;

	MessageBroker message_broker_;
	std::atomic <int> error_;
};

inline
void SchedulerMaster::received (void* data, DWORD size)
{
	switch (size) {
		case sizeof (SchedulerMessage::Tagged):
			switch (((const SchedulerMessage::Tagged*)data)->tag) {
				case SchedulerMessage::Tagged::CREATE_ITEM:
					Base::create_item ();
					break;
				case SchedulerMessage::Tagged::DELETE_ITEM:
					Base::delete_item ();
					break;
				case SchedulerMessage::Tagged::CORE_FREE:
					core_free ();
					break;
			}
		break;

		case sizeof (SchedulerMessage::Schedule) :
		{
			const SchedulerMessage::Schedule* msg = (const SchedulerMessage::Schedule*)data;
			Base::schedule (msg->deadline, msg->executor_id);
		}
		break;

		case sizeof (SchedulerMessage::ReSchedule) :
		{
			const SchedulerMessage::ReSchedule* msg = (const SchedulerMessage::ReSchedule*)data;
			Base::reschedule (msg->deadline, msg->executor_id, msg->deadline_prev);
		}
		break;
	}
}

}
}
}

#endif
