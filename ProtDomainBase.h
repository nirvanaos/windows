// Nirvana project.
// Windows implementation.
// Protection domain (process).

#ifndef NIRVANA_CORE_WINDOWS_PROTDOMAINBASE_H_
#define NIRVANA_CORE_WINDOWS_PROTDOMAINBASE_H_

#include "WorkerThreads.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class ProtDomainBase : 
	public WorkerThreads
{
};

}
}
}

#endif
