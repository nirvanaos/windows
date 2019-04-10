#include "../Port/ExecContext.h"

namespace Nirvana {
namespace Core {
namespace Port {

ExecContext::ExecContext (CreationType type) :
	fiber_ (nullptr)
{
	if (CREATE_NONE != type)
		fiber_ = CreateFiber (CREATE_NEUTRAL == type ? Windows::NEUTRAL_FIBER_STACK_SIZE : 0, fiber_proc, this);
}

void CALLBACK ExecContext::fiber_proc (void* param)
{
	// TODO: Implement
}

}
}
}
