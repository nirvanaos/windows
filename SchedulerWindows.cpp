// Nirvana project
// Windows implementation.
// SchedulerWindows class.

#include "../SchedulerImpl.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerWindows::received (OVERLAPPED* ovl, DWORD size)
{
	SchedulerMessage msg = *(SchedulerMessage*)data (ovl);
	enqueue_buffer (ovl);

	SchedulerImpl* scheduler = static_cast <SchedulerImpl*> (this);
	switch (msg.tag) {
	case SchedulerMessage::CORE_FREE:
		scheduler->core_free ();
		break;

	case SchedulerMessage::READY:
	case SchedulerMessage::UPDATE:
		{
			QueueItem item;
			item.process = (ProcessInfo*)msg.msg.ready.process;
			item.runnable = msg.msg.ready.runnable;
			scheduler->schedule (msg.msg.ready.deadline, item, SchedulerMessage::UPDATE == msg.tag);
		}
		break;

	//case SchedulerMessage::PROCESS_START:
	//case SchedulerMessage::PROCESS_STOP:

	default:
		assert (false);
	}
}

}
}
}
