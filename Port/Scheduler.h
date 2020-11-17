#ifndef NIRVANA_CORE_PORT_SCHEDULER_H_
#define NIRVANA_CORE_PORT_SCHEDULER_H_

#include "../Source/SchedulerAbstract.h"
#include <Runnable.h>

namespace Nirvana {
namespace Core {
namespace Port {

class Scheduler
{
	///@{
	/// Members called from Core.
public:
	static void create_item ()
	{
		return singleton_->create_item ();
	}

	static void delete_item () NIRVANA_NOEXCEPT
	{
		singleton_->delete_item ();
	}

	static void schedule (const DeadlineTime& deadline, Executor& executor) NIRVANA_NOEXCEPT
	{
		singleton_->schedule (deadline, executor);
	}

	static bool reschedule (const DeadlineTime& deadline, Executor& executor, const DeadlineTime& old) NIRVANA_NOEXCEPT
	{
		return singleton_->reschedule (deadline, executor, old);
	}

	static void shutdown () NIRVANA_NOEXCEPT
	{
		singleton_->shutdown ();
	}
	///@}

	static void worker_thread_proc ()
	{
		singleton_->worker_thread_proc ();
	}

protected:
	static Windows::SchedulerAbstract* singleton_;
};

}
}
}

#endif
