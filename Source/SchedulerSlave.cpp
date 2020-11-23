// TODO: Create watchdog thread which will wait for system domain process termination.
#include "SchedulerSlave.h"
#include <Scheduler.h>
#include "MailslotName.h"
#include "SchedulerMessage.h"

namespace Nirvana {
namespace Core {
namespace Windows {

SchedulerSlave::SchedulerSlave () :
	sys_process_ (nullptr),
	executor_id_ (0),
	error_ (0),
	queue_ (Port::SystemInfo::hardware_concurrency ())
{}

SchedulerSlave::SchedulerSlave (uint32_t sys_process_id, uint32_t sys_semaphore) :
	SchedulerSlave ()
{
	initialize (sys_process_id, sys_semaphore);
}

SchedulerSlave::~SchedulerSlave ()
{
	if (sys_process_)
		CloseHandle (sys_process_);
}

void SchedulerSlave::initialize (uint32_t sys_process_id, uint32_t sys_semaphore)
{
	if (!(sys_process_ = OpenProcess (SYNCHRONIZE | PROCESS_DUP_HANDLE, FALSE, sys_process_id)))
		throw_INITIALIZE ();

	HANDLE sem;
	if (!DuplicateHandle (sys_process_, (HANDLE)(uintptr_t)sys_semaphore, GetCurrentProcess (), &sem, 0, FALSE, DUPLICATE_SAME_ACCESS))
		throw_INTERNAL ();
	worker_threads_.semaphore (sem);
	executor_id_ = sys_semaphore;
}

inline
bool SchedulerSlave::initialize ()
{
	error_ = 0;

	DWORD process_id = GetCurrentProcessId ();
	message_broker_.create_mailslot (MailslotName (process_id));

	if (!(
		sys_mailslot_.open (MailslotName (0))
		&&
		scheduler_mailslot_.open (SCHEDULER_MAILSLOT_NAME)
	))
		return false; // System domain is not running

	try {
		sys_mailslot_.send (Message::ProcessStart (process_id));
	} catch (...) {
		return false; // System domain has just down
	}

	if (!sys_process_) {
		HANDLE hevent = CreateEventW (nullptr, FALSE, FALSE, nullptr);
		assert (hevent);
		bool success = false;
		OVERLAPPED ovl;
		memset (&ovl, 0, sizeof (ovl));
		ovl.hEvent = (HANDLE)((LONG_PTR)hevent | 1); // Avoid passing result to the completion port.

		Message::ProcessStartResponse ack;
		if (!ReadFile (message_broker_.mailslot_handle (), &ack, sizeof (ack), nullptr, &ovl)) {
			assert (ERROR_IO_PENDING == GetLastError ());

			if (WAIT_OBJECT_0 != WaitForSingleObject (hevent, PROCESS_START_ACK_TIMEOUT))
				CancelIoEx (message_broker_.mailslot_handle (), &ovl);
			else
				success = true;

			ovl.hEvent = hevent;
		} else
			success = true;

		DWORD size;
		GetOverlappedResult (message_broker_.mailslot_handle (), &ovl, &size, TRUE);
		CloseHandle (hevent);

		if (success) {
			if (sizeof (ack) != size || ack.type != Message::Type::PROCESS_START_RESPONSE || !ack.sys_process_id || !ack.executor_id)
				throw_INTERNAL ();

			initialize (ack.sys_process_id, ack.executor_id);
		} else
			return false;
	}
	return true;
}

void SchedulerSlave::terminate ()
{
	message_broker_.terminate ();
	scheduler_mailslot_.close ();
	if (sys_mailslot_.is_open ()) {
		try {
			sys_mailslot_.send (Message::ProcessStop (GetCurrentProcessId ()));
		} catch (...) {
		}
		sys_mailslot_.close ();
	}
	worker_threads_.terminate ();
}

bool SchedulerSlave::run (Runnable& startup, DeadlineTime startup_deadline)
{
	if (!initialize ())
		return false;
	try {
		message_broker_.start ();
		worker_threads_.run (startup, startup_deadline);
	} catch (...) {
		terminate ();
		throw;
	}
	terminate ();
	if (error_)
		CORBA::SystemException::_raise_by_code (error_);
	return true;
}

void SchedulerSlave::on_error (int err) NIRVANA_NOEXCEPT
{
	int zero = 0;
	if (error_.compare_exchange_strong (zero, err))
		Core::Scheduler::shutdown ();
}

void SchedulerSlave::create_item ()
{
	queue_.create_item ();
	try {
		scheduler_mailslot_.send (SchedulerMessage::Tagged (SchedulerMessage::Tagged::CREATE_ITEM));
	} catch (const CORBA::SystemException& ex) {
		on_error (ex.__code ());
		queue_.delete_item ();
		throw;
	}
}

void SchedulerSlave::delete_item () NIRVANA_NOEXCEPT
{
	queue_.delete_item ();
	try {
		scheduler_mailslot_.send (SchedulerMessage::Tagged (SchedulerMessage::Tagged::DELETE_ITEM));
	} catch (const CORBA::SystemException& ex) {
		on_error (ex.__code ());
	}
}

void SchedulerSlave::schedule (DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT
{
	try {
		queue_.insert (deadline, &executor);
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
	}

	try {
		scheduler_mailslot_.send (SchedulerMessage::Schedule (deadline, executor_id_));
	} catch (const CORBA::SystemException& ex) {
		on_error (ex.__code ());
	}
}

bool SchedulerSlave::reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) NIRVANA_NOEXCEPT
{
	if (error_)
		return false;

	try {
		if (!queue_.reorder (deadline, &executor, old))
			return false;
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
		return false;
	}

	try {
		scheduler_mailslot_.send (SchedulerMessage::ReSchedule (deadline, executor_id_, old));
	} catch (const CORBA::SystemException& ex) {
		on_error (ex.__code ());
		return false;
	}
	return true;
}

void SchedulerSlave::shutdown () NIRVANA_NOEXCEPT
{
	worker_threads_.shutdown ();
}

void SchedulerSlave::worker_thread_proc () NIRVANA_NOEXCEPT
{
	worker_threads_.thread_proc (*this);
}

inline
void SchedulerSlave::core_free () NIRVANA_NOEXCEPT
{
	if (!error_) {
		try {
			scheduler_mailslot_.send (SchedulerMessage::Tagged (SchedulerMessage::Tagged::CORE_FREE));
			return;
		} catch (const CORBA::SystemException& ex) {
			on_error (ex.__code ());
		}
	}

	assert (error_);
	// Fallback
	execute ();
}

void SchedulerSlave::execute () NIRVANA_NOEXCEPT
{
	Executor* executor;
	if (queue_.delete_min (executor))
		executor->execute ((CORBA::SystemException::Code)error_);
	core_free ();
}

}
}
}
