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
#include "SchedulerMessage.h"
#include "app_data.h"

#define DEBUG_SHUTDOWN

#ifdef DEBUG_SHUTDOWN
#include "../Port/Debugger.h"
#endif

namespace Nirvana {
namespace Core {
namespace Windows {

SchedulerSlave::SchedulerSlave () :
	sys_process_ (nullptr),
	terminate_event_ (nullptr),
	watchdog_thread_ (nullptr),
	executor_id_ (0),
	queue_ (Port::SystemInfo::hardware_concurrency ())
{}

SchedulerSlave::~SchedulerSlave ()
{
	if (watchdog_thread_) {
		SetEvent (terminate_event_);
		WaitForSingleObject (watchdog_thread_, INFINITE);
		CloseHandle (watchdog_thread_);
		watchdog_thread_ = nullptr;
	}
	scheduler_mailslot_.close ();
	worker_threads_.terminate ();
	if (sys_process_) {
		CloseHandle (sys_process_);
		sys_process_ = nullptr;
	}
	if (terminate_event_) {
		CloseHandle (terminate_event_);
		terminate_event_ = nullptr;
	}
}

bool SchedulerSlave::run (StartupProt& startup, DeadlineTime startup_deadline)
{
	if (!get_sys_process_id ())
		return false; // System domain is not running

	if (!(sys_process_ = OpenProcess (SYNCHRONIZE | PROCESS_DUP_HANDLE, FALSE, sys_process_id)))
		throw_COMM_FAILURE ();

	if (!scheduler_mailslot_.open (SCHEDULER_MAILSLOT_NAME))
		throw_COMM_FAILURE ();

	Mailslot watchdog_mailslot;
	if (!watchdog_mailslot.open (WATCHDOG_MAILSLOT_NAME))
		throw_COMM_FAILURE ();

	HANDLE sem = CreateSemaphoreW (nullptr, 0, (LONG)Port::SystemInfo::hardware_concurrency (), nullptr);
	worker_threads_.semaphore (sem);
	HANDLE executor;
	if (!DuplicateHandle (GetCurrentProcess (), sem, sys_process_, &executor, 0, FALSE, DUPLICATE_SAME_ACCESS))
		throw_INITIALIZE ();
	executor_id_ = (uint32_t)(uintptr_t)executor;

	if (!(terminate_event_ = CreateEventW (nullptr, TRUE, FALSE, nullptr)))
		throw_INITIALIZE ();

	if (!(watchdog_thread_ = CreateThread (nullptr, 0x10000, s_watchdog_thread_proc, this, 0, nullptr)))
		throw_INITIALIZE ();

	ProcessStartMessage process_start { GetCurrentProcessId (), executor_id_ };
	watchdog_mailslot.send (process_start);

	worker_threads_.run (startup, startup_deadline);

	return true;
}

void SchedulerSlave::create_item ()
{
	queue_.create_item ();
	try {
		scheduler_mailslot_.send (SchedulerMessage::Tagged (SchedulerMessage::Tagged::CREATE_ITEM));
	} catch (const CORBA::SystemException& ex) {
		on_error (ex.__code ());
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
	try {
		if (!queue_.reorder (deadline, &executor, old))
			return false;
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
	}

	try {
		scheduler_mailslot_.send (SchedulerMessage::ReSchedule (deadline, executor_id_, old));
	} catch (const CORBA::SystemException& ex) {
		on_error (ex.__code ());
	}
	return true;
}

void SchedulerSlave::shutdown () NIRVANA_NOEXCEPT
{
#ifdef DEBUG_SHUTDOWN
	Port::Debugger::output_debug_string ("Shutdown 2\n");
#endif
	worker_threads_.shutdown ();
}

void SchedulerSlave::worker_thread_proc () NIRVANA_NOEXCEPT
{
	worker_threads_.thread_proc (*this);
}

inline
void SchedulerSlave::core_free () NIRVANA_NOEXCEPT
{
	try {
		scheduler_mailslot_.send (SchedulerMessage::Tagged (SchedulerMessage::Tagged::CORE_FREE));
		return;
	} catch (const CORBA::SystemException& ex) {
		on_error (ex.__code ());
	}
}

void SchedulerSlave::execute () NIRVANA_NOEXCEPT
{
	Executor* executor;
	if (queue_.delete_min (executor))
		ThreadWorker::execute (*executor);
	core_free ();
}

DWORD WINAPI SchedulerSlave::s_watchdog_thread_proc (void* _this)
{
	SchedulerSlave* p = (SchedulerSlave*)_this;
	HANDLE wait_handles [2] = { p->terminate_event_, p->sys_process_ };
	if ((WAIT_OBJECT_0 + 1) == WaitForMultipleObjects (2, wait_handles, FALSE, INFINITE)) {
		// System process was terminated unexpectedly.
		DWORD exit_code = -1;
		GetExitCodeProcess (wait_handles [1], &exit_code);
		ExitProcess (exit_code);
	}
	return 0;
}

}
}
}
