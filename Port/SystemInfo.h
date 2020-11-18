// Nirvana project
// Windows implementation.
// Windows system information.

#ifndef NIRVANA_CORE_PORT_SYSTEMINFO_H_
#define NIRVANA_CORE_PORT_SYSTEMINFO_H_

#include <core.h>

namespace Nirvana {
namespace Core {
namespace Port {

class SystemInfo
{
public:
	static unsigned int hardware_concurrency () NIRVANA_NOEXCEPT
	{
		return 1;
		//return singleton_.hardware_concurrency_;
	}

private:
	SystemInfo ();

private:
	static SystemInfo singleton_;

	unsigned int hardware_concurrency_;
};


}
}
}

#endif


