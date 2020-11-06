#ifndef NIRVANA_CORE_PORT_BACKOFF_H_
#define NIRVANA_CORE_PORT_BACKOFF_H_

#include <Nirvana/Nirvana.h>

extern "C" __declspec (dllimport)
void __stdcall Sleep (unsigned long dwMilliseconds);

extern "C" __declspec (dllimport)
int __stdcall SwitchToThread (void);

namespace Nirvana {
namespace Core {
namespace Port {

class BackOff
{
protected:
	/// A number of iterations when we should call yield ().
	/// SwitchToThread () experiences the expensive cost of a context switch, which can be 10000+ cycles.
	/// It also suffers the cost of ring 3 to ring 0 transitions, which can be 1000+ cycles.
	static const UWord ITERATIONS_YIELD = 11000;

	/// Maximal number of iterations.
	static const UWord ITERATIONS_MAX = 1000000;

	static void yield (UWord iterations)
	{
		::SwitchToThread ();
	}
};

}
}
}

#endif
