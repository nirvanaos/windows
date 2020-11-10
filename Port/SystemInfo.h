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
	SystemInfo ();

	unsigned int hardware_concurrency ()
	{
		return hardware_concurrency_;
	}

private:
	unsigned int hardware_concurrency_;
};

static SystemInfo g_system_info;

}
}
}

#endif


