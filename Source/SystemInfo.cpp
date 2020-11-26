#include "../Port/SystemInfo.h"
#include <PortableExecutable.h>
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

bool SystemInfo::get_OLF_section (Section& section) NIRVANA_NOEXCEPT
{
	PortableExecutable pe (GetModuleHandleW (nullptr));
	return pe.find_OLF_section (section);
}

}
}
}
