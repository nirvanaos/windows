// Nirvana project
// Windows implementation.
// SchedulerMaster class. 

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERMASTER_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERMASTER_H_

#include "PostOffice.h"
#include "Mailslot.h"
#include "WorkerThreads.h"
#include <SchedulerImpl.h>
#include "SchedulerMessage.h"
#include "MessageBroker.h"
#include "RoundBuffers.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerItem
{
public:
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
	static const uintptr_t IS_SEMAPHORE = 1 << sizeof (uintptr_t) * 8 - 1;
	uintptr_t executor_;
};

class SchedulerMaster :
	public PostOffice <SchedulerMaster, sizeof (SchedulerMessage::Buffer), SCHEDULER_THREAD_PRIORITY>,
	public SchedulerImpl <SchedulerMaster, SchedulerItem>
{
public:
	typedef SchedulerImpl <SchedulerMaster, SchedulerItem> Base;
	typedef PostOffice <SchedulerMaster, sizeof (SchedulerMessage::Buffer), SCHEDULER_THREAD_PRIORITY> Office;

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

	void core_free ();

	static void core_free_static ();

	/// Process mailslot message.
	void received (void* data, DWORD size);

protected:
	virtual void execute () NIRVANA_NOEXCEPT;

private:
	static SchedulerMaster& scheduler ()
	{
		assert (scheduler_);
		return static_cast <SchedulerMaster&> (*scheduler_);
	}

private:
	/// Helper class for executing in the current process.
	class InProcExecute :
		public CompletionPortReceiver
	{
	public:
		InProcExecute (unsigned thread_count) :
			buffers_ (thread_count)
		{}

		~InProcExecute ()
		{}

		void execute (const Execute& msg)
		{
			Execute* buffer = buffers_.next_buffer ();
			*buffer = msg;
			worker_threads_.post (*this, reinterpret_cast <OVERLAPPED*> (buffer), 0);
		}

		void run (Runnable& startup, DeadlineTime deadline)
		{
			worker_threads_.run (startup, deadline);
		}

		void shutdown ()
		{
			worker_threads_.shutdown ();
		}

	private:
		virtual void received (OVERLAPPED* ovl, DWORD size);

		RoundBuffers <Execute> buffers_;
		WorkerThreads worker_threads_;
	}
	in_proc_execute_;

	MessageBroker message_broker_;
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
