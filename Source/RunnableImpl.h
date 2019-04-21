#ifndef NIRVANA_CORE_WINDOWS_RUNNABLEIMPL_H_
#define NIRVANA_CORE_WINDOWS_RUNNABLEIMPL_H_

#include <Nirvana/Runnable_s.h>

namespace Nirvana {
namespace Core {
namespace Windows {

template <class S>
class RunnableImpl :
	public CORBA::Nirvana::Servant <S, Runnable>,
	public CORBA::Nirvana::LifeCycleStatic <>
{};

}
}
}


#endif
