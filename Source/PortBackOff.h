#ifndef NIRVANA_CORE_PORTBACKOFF_H_
#define NIRVANA_CORE_PORTBACKOFF_H_

#include "win32.h"

namespace Nirvana {
namespace Core {

class PortBackOff
{
protected:
	static void sleep (unsigned hint)
	{
		::Sleep (hint);
	}
};

}
}

#endif
