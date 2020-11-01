#include "SchedulerClient.h"
#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerClient::schedule (DeadlineTime deadline, Executor& executor, DeadlineTime deadline_prev, bool nothrow_fallback)
{
	SchedulerMessage msg;
	msg.tag = SchedulerMessage::SCHEDULE;
	msg.msg.schedule.protection_domain = protection_domain_;
	msg.msg.schedule.executor = (uint64_t)&executor;
	msg.msg.schedule.deadline = deadline;
	msg.msg.schedule.deadline_prev = deadline_prev;
	try {
		send (msg);
	} catch (...) {
		if (nothrow_fallback) {
			// Fallback
			Execute* exec = fallback_buffers_.next_buffer ();
			exec->executor = msg.msg.schedule.executor;
			exec->deadline = msg.msg.schedule.deadline;
			exec->scheduler_error = CORBA::SystemException::EC_COMM_FAILURE;
			enqueue_buffer ((OVERLAPPED*)exec);
		} else
			throw;
	}
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
		ThreadWorker::execute (*executor, exec.deadline, (Word)exec.scheduler_error);
}

}
}
}
