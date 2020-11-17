#include "../Port/SystemInfo.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

SystemInfo SystemInfo::singleton_;

SystemInfo::SystemInfo ()
{
	::SYSTEM_INFO si;
	::GetSystemInfo (&si);
	hardware_concurrency_ =  si.dwNumberOfProcessors;
}

}
}
}
