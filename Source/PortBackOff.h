#ifndef NIRVANA_CORE_PORT_BACKOFF_H_
#define NIRVANA_CORE_PORT_BACKOFF_H_

#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

class BackOff
{
protected:
	static void sleep (unsigned hint)
	{
		::Sleep (hint);
	}
};

}
}
}

#endif
