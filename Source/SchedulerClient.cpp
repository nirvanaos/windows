#include "SchedulerClient.h"
#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerClient::schedule (DeadlineTime deadline, Executor& executor, DeadlineTime deadline_prev, bool nothrow_fallback)
{
	try {
		queue_.insert (deadline, &executor);
	} catch (...) {
		if (deadline_prev)
			queue_.erase (deadline_prev, &executor);
		if (nothrow_fallback) {
			fallback (executor, deadline, CORBA::SystemException::EC_NO_MEMORY);
			return;
		} else
			throw;
	}
	if (deadline_prev)
		queue_.erase (deadline_prev, &executor);

	SchedulerMessage msg;
	msg.tag = SchedulerMessage::SCHEDULE;
	msg.msg.schedule.protection_domain = protection_domain_;
	msg.msg.schedule.executor = (uint64_t)&executor;
	msg.msg.schedule.deadline = deadline;
	msg.msg.schedule.deadline_prev = deadline_prev;

	try {
		send (msg);
	} catch (...) {
		queue_.erase (deadline, &executor);
		if (nothrow_fallback)
			fallback (executor, deadline, CORBA::SystemException::EC_COMM_FAILURE);
		else
			throw;
	}
}

void SchedulerClient::fallback (Executor& executor, DeadlineTime deadline, int error)
{
	Execute* exec = fallback_buffers_.next_buffer ();
	exec->executor = (uint64_t)&executor;
	exec->deadline = deadline;
	exec->scheduler_error = error;
	worker_threads_.post (*this, reinterpret_cast <OVERLAPPED*> (exec), 0);
}

void SchedulerClient::core_free ()
{
	SchedulerMessage msg;
	msg.tag = SchedulerMessage::CORE_FREE;
	try {
		send (msg);
	} catch (...) {
		// Fallback
		Executor* executor;
		DeadlineTime deadline;
		if (queue_.delete_min (executor, deadline))
			fallback (*executor, deadline, CORBA::SystemException::EC_COMM_FAILURE);
	}
}

void SchedulerClient::send (const SchedulerMessage& msg)
{
	scheduler_mailslot_.send (&msg, msg.size ());
}

void SchedulerClient::received (OVERLAPPED* ovl, DWORD size)
{
	Execute exec = { 0 };
	Executor* executor;
	if (BufferPool::begin () <= ovl && ovl < BufferPool::end ()) {
		// Message is from the mailslot.
		if (sizeof (Execute) == size)
			exec = *(Execute*)data (ovl);
		enqueue_buffer (ovl);
		if (!exec.executor)
			return;
		executor = reinterpret_cast <Executor*> (exec.executor);
		if (!queue_.erase (exec.deadline, executor))
			return;
	} else {
		// Fallback
		Execute* pexec = (Execute*)ovl;
		assert (fallback_buffers_.from_here (pexec));
		exec = *pexec;
		executor = reinterpret_cast <Executor*> (exec.executor);
	}
	ThreadWorker::execute (*executor, exec.deadline, (Word)exec.scheduler_error);
}

}
}
}
