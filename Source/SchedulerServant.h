#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERSERVANT_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERSERVANT_H_

#include <Scheduler_s.h>
#include <core.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

template <class S>
class SchedulerServant :
	public CoreObject,
	public ::CORBA::Nirvana::Servant <S, Scheduler>,
	public ::CORBA::Nirvana::LifeCycleStatic <>
{};

}
}
}

#endif
