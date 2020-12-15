#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERBASE_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERBASE_H_

#include "../Port/Scheduler.h"
#include "SchedulerAbstract.h"

namespace Nirvana {
namespace Core {
namespace Windows {

template <class Impl>
class NIRVANA_NOVTABLE SchedulerBase :
	public SchedulerAbstract,
	public Port::Scheduler
{
public:
	SchedulerBase ()
	{
		singleton_ = this;
	}

	~SchedulerBase ()
	{
		singleton_ = nullptr;
	}

	static Impl& singleton ()
	{
		assert (singleton_);
		//assert (dynamic_cast <Impl*> (singleton_));
		return static_cast <Impl&> (*singleton_);
	}
};

}
}
}

#endif
