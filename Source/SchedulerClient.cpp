#include "SchedulerClient.h"
#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerClient::_schedule (::CORBA::Nirvana::Bridge <Scheduler>* bridge,
																 DeadlineTime deadline, ::CORBA::Nirvana::BridgeMarshal <Executor>* executor,
																 DeadlineTime deadline_prev, ::CORBA::Nirvana::EnvironmentBridge*)
{
	SchedulerClient* _this = static_cast <SchedulerClient*> (bridge);
	SchedulerMessage msg;
	msg.tag = SchedulerMessage::SCHEDULE;
	msg.msg.schedule.protection_domain = _this->protection_domain_;
	msg.msg.schedule.executor = (uint64_t)executor;
	msg.msg.schedule.deadline = deadline;
	msg.msg.schedule.deadline_prev = deadline_prev;
	_this->send (msg);
}

void SchedulerClient::_core_free (::CORBA::Nirvana::Bridge <Scheduler>* bridge, ::CORBA::Nirvana::EnvironmentBridge*)
{
	SchedulerMessage msg;
	msg.tag = SchedulerMessage::CORE_FREE;
	static_cast <SchedulerClient*> (bridge)->send (msg);
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
	
	::CORBA::Nirvana::Bridge <Executor>* executor = reinterpret_cast < ::CORBA::Nirvana::Bridge <Executor>*> (exec.executor);
	if (executor)
		Thread::execute (executor, exec.deadline);
}

}
}
}
