// Nirvana project.
// Windows implementation.
// CompletionPortReceiver abstract class.

#ifndef NIRVANA_CORE_WINDOWS_COMPLETIONPORTRECEIVER_H_
#define NIRVANA_CORE_WINDOWS_COMPLETIONPORTRECEIVER_H_

#include <core.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class NIRVANA_NOVTABLE CompletionPortReceiver
{
public:
	virtual void received (OVERLAPPED* ovl, DWORD size) = 0;
};

}
}
}

#endif
