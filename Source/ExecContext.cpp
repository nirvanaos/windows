#include <ExecDomain.h>
#include "../Port/ProtDomainMemory.h"

namespace Nirvana {
namespace Core {
namespace Port {

void __stdcall ExecContext::fiber_proc (void*)
{
#if !(defined (__GNUG__) || defined (__clang__))
	_set_se_translator (&ProtDomainMemory::se_translator);
#endif
	Core::Thread::current ().execution_domain ()->execute_loop ();
}

}
}
}
