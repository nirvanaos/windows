#include <ExecDomain.h>
#include <CORBA/Exception.h>

namespace Nirvana {
namespace Core {
namespace Port {

ExecContext::ExecContext (CreationType type) :
	fiber_ (nullptr)
{
	switch (type) {
	case CREATE_NONE:
		return;

	case CREATE_CONVERT:
		fiber_ = ConvertThreadToFiber (this);
		break;

	case CREATE_DEFAULT:
		fiber_ = CreateFiber (0, fiber_proc, this);
		break;

	default:
		assert (false);
	}

	if (!fiber_)
		throw CORBA::NO_MEMORY ();
}

void CALLBACK ExecContext::fiber_proc (void* param)
{
	static_cast <ExecDomain&> (*(ExecContext*)param).execute_loop ();
}

}
}
}
