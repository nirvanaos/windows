#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERABSTRACT_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERABSTRACT_H_

#include "Executor.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerAbstract
{
public:
	virtual void schedule (DeadlineTime deadline, Executor& executor, DeadlineTime old, bool nothrow_fallback) = 0;
	virtual void core_free () = 0;
	virtual void shutdown () = 0;
};

}
}
}

#endif
