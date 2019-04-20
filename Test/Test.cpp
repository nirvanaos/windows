#include <core.h>
#include <CORBA/Servant.h>

namespace Nirvana {
namespace Core {

Memory_ptr g_core_heap;

}
}

namespace CORBA {
namespace Nirvana {

ObjectFactory_ptr g_object_factory = ObjectFactory_ptr::nil ();

}
}
