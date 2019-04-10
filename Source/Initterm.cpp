#include <InitTerm.h>
#include <core.h>
#include "MemoryWindows.h"

namespace Nirvana {
namespace Core {

void initialize_memory ()
{
	Windows::MemoryWindows::initialize ();
	g_protection_domain_memory = Windows::MemoryWindows::_this ();
}

void terminate_memory ()
{
	g_protection_domain_memory = Memory::_nil ();
	Windows::MemoryWindows::terminate ();
}

}
}
