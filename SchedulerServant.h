#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERSERVANT_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERSERVANT_H_

#include "win32.h"
#include "../../Interface/Scheduler.h"
#include "../core.h"

namespace Nirvana {
namespace Core {
namespace Windows {

template <class S>
class SchedulerServant :
	public CoreObject,
	public ::CORBA::Nirvana::Servant <S, Scheduler>
{
public:
	static void _back_off (::CORBA::Nirvana::Bridge <Scheduler>*, ::CORBA::ULong hint, ::CORBA::Nirvana::EnvironmentBridge*)
	{
		Sleep (hint);
	}
};

}
}
}

#endif
