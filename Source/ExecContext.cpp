#include <ExecDomain.h>
#include <CORBA/Exception.h>

namespace Nirvana {
namespace Core {
namespace Port {

void CALLBACK ExecContext::fiber_proc (void*)
{
	Core::Thread::current ().execution_domain ()->execute_loop ();
}

}
}
}
