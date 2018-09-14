#include "ExecContextWindows.h"

namespace Nirvana {
namespace Core {
namespace Windows {

ExecContextWindows::ExecContextWindows (CreationType type) :
	fiber_ (nullptr)
{
	if (CREATE_NONE != type)
		fiber_ = CreateFiber (CREATE_NEUTRAL == type ? NEUTRAL_FIBER_STACK_SIZE : 0, fiber_proc, this);
}
/*
void CALLBACK ExecContextWindows::fiber_proc (void* param)
{
	__try {

	} __except ()
	{

	}
}
*/
}
}
}
