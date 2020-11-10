#include "../Port/SystemInfo.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

SystemInfo g_system_info;

SystemInfo::SystemInfo ()
{
	::SYSTEM_INFO si;
	::GetSystemInfo (&si);
	hardware_concurrency_ =  si.dwNumberOfProcessors;
}

}
}
}
