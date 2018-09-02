// Nirvana project.
// Windows implementation.
// System domain (computer).

#ifndef NIRVANA_CORE_WINDOWS_SYSDOMAINBASE_H_
#define NIRVANA_CORE_WINDOWS_SYSDOMAINBASE_H_

#include "Scheduler.h"

namespace Nirvana {
namespace Core {

class SysDomain;

namespace Windows {

class SysDomainBase :
	public Scheduler
{
public:
	static void initialize ();
	static void terminate ();

	static SysDomain* singleton_; // Temporary solution
};

}
}
}

#endif
