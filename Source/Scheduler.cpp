#include "../Port/Scheduler.h"
#include "SchedulerMaster.h"
#include "SchedulerSlave.h"

namespace Nirvana {
namespace Core {
namespace Port {

Windows::SchedulerAbstract* Scheduler::singleton_ = nullptr;

}
}
}
