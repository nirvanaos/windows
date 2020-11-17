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
	/// \param argc Command line argumens count.
	/// \param argv Command line argumens.
	/// \returns `false` if system domain is already running.
	bool run (int argc, char* argv []);

	// Implementation of SchedulerAbstract.
	virtual void create_item ();
	virtual void delete_item () NIRVANA_NOEXCEPT;
	virtual void schedule (DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT;
	virtual bool reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) NIRVANA_NOEXCEPT;
	virtual void shutdown () NIRVANA_NOEXCEPT;

	/// Process mailslot message.
	void received (void* data, DWORD size) NIRVANA_NOEXCEPT;

	/// Called by SchedulerImpl
	void execute (SchedulerItem& item) NIRVANA_NOEXCEPT
	{
		if (item.is_semaphore ())
			ReleaseSemaphore (item.semaphore (), 1, nullptr);
		else
			worker_threads_.execute (item.executor ());
	}

private:
	void on_error (int err) NIRVANA_NOEXCEPT;

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
		virtual void received (OVERLAPPED* ovl, DWORD size) NIRVANA_NOEXCEPT;

	}
	worker_threads_;

	MessageBroker message_broker_;
	std::atomic <int> error_;
};

inline
void SchedulerMaster::received (void* data, DWORD size) NIRVANA_NOEXCEPT
{
	switch (size) {
		case sizeof (SchedulerMessage::Tagged):
			switch (((const SchedulerMessage::Tagged*)data)->tag) {
				case SchedulerMessage::Tagged::CREATE_ITEM:
					try {
						Base::create_item ();
					} catch (...) {
						on_error (CORBA::SystemException::EC_NO_MEMORY);
					}
					break;
				case SchedulerMessage::Tagged::DELETE_ITEM:
					Base::delete_item ();
					break;
				case SchedulerMessage::Tagged::CORE_FREE:
					core_free ();
					break;
			}
		break;

		case sizeof (SchedulerMessage::Schedule) : {
			const SchedulerMessage::Schedule* msg = (const SchedulerMessage::Schedule*)data;
			try {
				Base::schedule (msg->deadline, msg->executor_id);
			} catch (...) {
				on_error (CORBA::SystemException::EC_NO_MEMORY);
				// Fallback
				HANDLE h = (HANDLE)msg->executor_id;
				if (WAIT_OBJECT_0 == WaitForSingleObject (h, 0))
					Base::free_cores_.increment ();
				if (ReleaseSemaphore (h, 1, nullptr))
					Base::free_cores_.decrement ();
			}
		}
		break;

		case sizeof (SchedulerMessage::ReSchedule) : {
			const SchedulerMessage::ReSchedule* msg = (const SchedulerMessage::ReSchedule*)data;
			try {
				Base::reschedule (msg->deadline, msg->executor_id, msg->deadline_prev);
			} catch (...) {
				on_error (CORBA::SystemException::EC_NO_MEMORY);
			}
		}
		break;
	}
}

}
}
}

#endif
