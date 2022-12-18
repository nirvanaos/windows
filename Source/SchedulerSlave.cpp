// TODO: Create watchdog thread which will wait for system domain process termination.
/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include "SchedulerSlave.h"
#include <Scheduler.h>
#include <StartupProt.h>
#include <Executor.h>
#include "MailslotName.h"
#include "SchedulerMessage.h"
#include "sysdomainid.h"

namespace Nirvana {
namespace Core {
namespace Windows {

SchedulerSlave::SchedulerSlave () :
	sys_process_ (nullptr),
	executor_id_ (0),
	error_ (CORBA::Exception::EC_NO_EXCEPTION),
	queue_ (Port::SystemInfo::hardware_concurrency ())
{}

void SchedulerSlave::terminate ()
{
	message_broker_.terminate ();
	scheduler_mailslot_.close ();
	worker_threads_.terminate ();
	if (sys_process_)
		CloseHandle (sys_process_);
}

bool SchedulerSlave::run (StartupProt& startup, DeadlineTime startup_deadline)
{
	DWORD process_id = GetCurrentProcessId ();
	message_broker_.create_mailslot (MailslotName (process_id));

	if (!get_sys_process_id ())
		return false; // System domain is not running

	if (!(sys_process_ = OpenProcess (SYNCHRONIZE | PROCESS_DUP_HANDLE, FALSE, sys_process_id)))
		return false; // System domain is not running

	if (!scheduler_mailslot_.open (SCHEDULER_MAILSLOT_NAME))
		return false; // System domain is not running

	Mailslot watchdog_mailslot;
	if (!watchdog_mailslot.open (WATCHDOG_MAILSLOT_NAME))
		return false; // System domain is not running

	HANDLE sem = CreateSemaphoreW (nullptr, 0, (LONG)Port::SystemInfo::hardware_concurrency (), nullptr);
	worker_threads_.semaphore (sem);
	HANDLE executor;
	if (!DuplicateHandle (GetCurrentProcess (), sem, sys_process_, &executor, 0, FALSE, DUPLICATE_SAME_ACCESS))
		throw_INITIALIZE ();
	executor_id_ = (uint32_t)(uintptr_t)executor;

	try {
		ProcessStartMessage process_start{ GetCurrentProcessId (), executor_id_ };
		watchdog_mailslot.send (process_start);
		message_broker_.start ();
		worker_threads_.run (startup, startup_deadline);
	} catch (...) {
		terminate ();
		throw;
	}
	if (error_ >= 0)
		CORBA::SystemException::_raise_by_code (error_);
	return true;
}

void SchedulerSlave::on_error (int err) NIRVANA_NOEXCEPT
{
	int zero = CORBA::Exception::EC_NO_EXCEPTION;
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
	if (error_ >= 0)
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
	if (error_ < 0) {
		try {
			scheduler_mailslot_.send (SchedulerMessage::Tagged (SchedulerMessage::Tagged::CORE_FREE));
			return;
		} catch (const CORBA::SystemException& ex) {
			on_error (ex.__code ());
		}
	}

	assert (error_ >= 0);
	// Fallback
	execute ();
}

void SchedulerSlave::execute () NIRVANA_NOEXCEPT
{
	Executor* executor;
	if (queue_.delete_min (executor))
		executor->execute (error_);
	core_free ();
}

}
}
}
