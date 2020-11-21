#include "../Port/Scheduler.h"
#include "SchedulerMaster.h"
#include "SchedulerSlave.h"
#include <iostream>

namespace Nirvana {
namespace Core {
namespace Port {

using namespace std;

Windows::SchedulerAbstract* Scheduler::singleton_ = nullptr;

}
}
}
