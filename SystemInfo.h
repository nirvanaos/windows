// Nirvana project
// Windows implementation.
// Windows system information.

#ifndef NIRVANA_CORE_WINDOWS_SYSTEMINFO_H_
#define NIRVANA_CORE_WINDOWS_SYSTEMINFO_H_

#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SystemInfo
{
public:
	static unsigned int hardware_concurrency ()
	{
		::SYSTEM_INFO si;
		::GetSystemInfo (&si);
		return si.dwNumberOfProcessors;
	}
};

}
}
}

#endif


