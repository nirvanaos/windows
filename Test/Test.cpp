#include <core.h>
#include <CORBA/Servant.h>

namespace Nirvana {
namespace Core {

Memory_ptr g_core_heap;

}
}

namespace CORBA {
namespace Nirvana {

Bridge <ObjectFactory>* const g_object_factory = nullptr;

}
}
