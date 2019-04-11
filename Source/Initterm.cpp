#include <InitTerm.h>
#include <core.h>
#include "../Port/ProtDomainMemory.h"

namespace Nirvana {
namespace Core {

void initialize_memory ()
{
	Port::ProtDomainMemory::initialize ();
	g_protection_domain_memory = Port::ProtDomainMemory::_this ();
}

void terminate_memory ()
{
	g_protection_domain_memory = Memory::_nil ();
	Port::ProtDomainMemory::terminate ();
}

}
}
