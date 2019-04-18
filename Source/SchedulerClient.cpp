#include "SchedulerClient.h"
#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerClient::schedule (DeadlineTime deadline, Executor& executor, DeadlineTime deadline_prev)
{
	SchedulerMessage msg;
	msg.tag = SchedulerMessage::SCHEDULE;
	msg.msg.schedule.protection_domain = protection_domain_;
	msg.msg.schedule.executor = (uint64_t)&executor;
	msg.msg.schedule.deadline = deadline;
	msg.msg.schedule.deadline_prev = deadline_prev;
	send (msg);
}

void SchedulerClient::core_free ()
{
	SchedulerMessage msg;
	msg.tag = SchedulerMessage::CORE_FREE;
	send (msg);
}

void SchedulerClient::send (const SchedulerMessage& msg)
{
	scheduler_mailslot_.send (&msg, msg.size ());
}

void SchedulerClient::received (OVERLAPPED* ovl, DWORD size)
{
	assert (sizeof (Execute) == size);
	Execute exec = {0};
	if (sizeof (Execute) == size)
		exec = *(Execute*)data (ovl);
	enqueue_buffer (ovl);
	
	Executor* executor = reinterpret_cast <Executor*> (exec.executor);
	if (executor)
		Thread::execute (*executor, exec.deadline);
}

}
}
}
