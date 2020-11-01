#include <core.h>

namespace Nirvana {
namespace Core {
namespace Port {

NIRVANA_NORETURN void _unrecoverable_error (const char* file, unsigned line)
{
	assert (false);
	abort ();
}

}
}
}
