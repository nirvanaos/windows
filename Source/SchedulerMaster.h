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

namespace Nirvana {
namespace Core {
namespace Windows {

struct SchedulerItem
{
	uint32_t semaphore;

	bool operator < (const SchedulerItem& rhs) const
	{
		// Each item must be unique even if executors are the same.
		return this < &rhs;
	}
};

class SchedulerMaster :
	public PostOffice <SchedulerMaster, sizeof (SchedulerMessage::Buffer), SCHEDULER_THREAD_PRIORITY>,
	public SchedulerImpl <SchedulerMaster, SchedulerItem>,
	public WorkerThreads
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
	}
	SchedulerMessage* msg = (SchedulerMessage*)data;

	switch (msg->tag) {
	case SchedulerMessage::CORE_FREE:
		Base::core_free ();
		break;

	case SchedulerMessage::SCHEDULE:
	{
		SchedulerItem item (msg->msg.schedule.protection_domain, msg->msg.schedule.executor);
		try {
			Base::schedule (msg->msg.schedule.deadline, item, msg->msg.schedule.deadline_prev);
		} catch (...) {
			// Fallback
			Execute exec;
			exec.executor = item.executor;
			exec.deadline = msg->msg.schedule.deadline;
			exec.scheduler_error = CORBA::SystemException::EC_NO_MEMORY;
			item.protection_domain->port ().execute (exec);
		}
	}
		break;

	case SchedulerMessage::PROCESS_START:
		reinterpret_cast <SysDomain::ProtDomainInfo*> (msg->msg.process_start.protection_domain)->port ().process_start (msg->msg.process_start.process_id);
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
