// Nirvana project.
// Windows implementation.
// Protection domain (process).

#include "ProtDomainBase.h"

namespace Nirvana {
namespace Core {
namespace Windows {

HANDLE ProtDomainBase::scheduler_mailslot_;
HANDLE ProtDomainBase::run_mailslot_;
HANDLE ProtDomainBase::free_cores_semaphore_;

}
}
}
