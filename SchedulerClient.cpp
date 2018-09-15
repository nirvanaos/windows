#include "SchedulerClient.h"
#include "../Thread.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void SchedulerClient::_schedule (::CORBA::Nirvana::Bridge <Scheduler>* bridge,
																 DeadlineTime deadline, ::CORBA::Nirvana::Bridge <Runnable>* runnable,
																 DeadlineTime deadline_prev, ::CORBA::Nirvana::EnvironmentBridge*)
{
	SchedulerClient* _this = static_cast <SchedulerClient*> (bridge);
	SchedulerMessage msg;
	msg.tag = SchedulerMessage::SCHEDULE;
	msg.msg.schedule.process = _this->process_;
	msg.msg.schedule.runnable = (uint64_t)runnable;
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
	Execute exec = {0};
	if (sizeof (Execute) == size)
		exec = *(Execute*)data (ovl);
	enqueue_buffer (ovl);
	
	::CORBA::Nirvana::Bridge <Runnable>* p = reinterpret_cast < ::CORBA::Nirvana::Bridge <Runnable>*> (exec.runnable);
	Thread::execute (p);
}

}
}
}
