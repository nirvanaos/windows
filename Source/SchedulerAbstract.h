#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERABSTRACT_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERABSTRACT_H_

#include <Executor.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class NIRVANA_NOVTABLE SchedulerAbstract
{
public:
	virtual void create_item () = 0;
	virtual void delete_item () NIRVANA_NOEXCEPT = 0;
	virtual void schedule (DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT = 0;
	virtual bool reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) NIRVANA_NOEXCEPT = 0;
	virtual void shutdown () NIRVANA_NOEXCEPT = 0;
	virtual void worker_thread_proc () NIRVANA_NOEXCEPT = 0;
};

}
}
}

#endif
