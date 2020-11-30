#ifndef NIRVANA_CORE_PORT_SCHEDULER_H_
#define NIRVANA_CORE_PORT_SCHEDULER_H_

#include "../Source/SchedulerAbstract.h"

namespace Nirvana {
namespace Core {
namespace Port {

class Scheduler
{
	///@{
	/// Members called from Core.
public:
	/// Reserve space for an active item.
	/// \throws CORBA::NO_MEMORY
	static void create_item ()
	{
		return singleton_->create_item ();
	}

	/// Release active item space.
	static void delete_item () NIRVANA_NOEXCEPT
	{
		if (singleton_)
			singleton_->delete_item ();
	}

	/// \summary Schedule execution.
	/// 
	/// \param deadline Deadline.
	/// \param executor Executor.
	static void schedule (const DeadlineTime& deadline, Executor& executor) NIRVANA_NOEXCEPT
	{
		singleton_->schedule (deadline, executor);
	}

	/// \summary Re-schedule execution.
	/// 
	/// \param deadline New deadline.
	/// \param executor Executor.
	/// \param old Old deadline.
	/// \returns `true` if the executor was found and rescheduled. `false` if executor with old deadline was not found.
	static bool reschedule (const DeadlineTime& deadline, Executor& executor, const DeadlineTime& old) NIRVANA_NOEXCEPT
	{
		return singleton_->reschedule (deadline, executor, old);
	}

	/// Initiate shutdown for the current domain.
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
