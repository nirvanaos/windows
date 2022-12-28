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
#include "SchedulerMaster.h"
#include "../Port/Scheduler.h"
#include "MailslotName.h"
#include "sysdomainid.h"
#include <StartupSys.h>

namespace Nirvana {
namespace Core {
namespace Windows {

SchedulerMaster::SchedulerMaster () :
	sysdomainid_ (INVALID_HANDLE_VALUE),
	watchdog_ (*this),
	error_ (CORBA::Exception::EC_NO_EXCEPTION)
{}

void SchedulerMaster::terminate ()
{
	if (INVALID_HANDLE_VALUE != sysdomainid_) {
		CloseHandle (sysdomainid_);
		sysdomainid_ = INVALID_HANDLE_VALUE;
	}
	Office::terminate ();
	message_broker_.terminate ();
	worker_threads_.terminate ();
	watchdog_.terminate ();
}

bool SchedulerMaster::run (StartupSys& startup)
{
	sys_process_id = GetCurrentProcessId ();
	HANDLE sysdomainid = open_sysdomainid (true);
	if (INVALID_HANDLE_VALUE == sysdomainid)
		return false;
	{
		DWORD written;
		if (!WriteFile (sysdomainid, &sys_process_id, sizeof (DWORD), &written, nullptr))
			throw_INITIALIZE ();
	}

	if (!(
		Office::create_mailslot (SCHEDULER_MAILSLOT_NAME)
		&&
		message_broker_.create_mailslot (MailslotName (sys_process_id))
		))
		return false;

	if (!watchdog_.start ())
		return false;

	try {
		Office::start ();
		message_broker_.start ();
		worker_threads_.run (startup, INFINITE_DEADLINE);
	} catch (...) {
		terminate ();
		throw;
	}
	if (error_ >= 0)
		CORBA::SystemException::_raise_by_code (error_);
	return true;
}

void SchedulerMaster::on_error (int err) NIRVANA_NOEXCEPT
{
	int zero = CORBA::Exception::EC_NO_EXCEPTION;
	if (error_.compare_exchange_strong (zero, err))
		ExitProcess (err);
}

void SchedulerMaster::create_item ()
{
	Base::create_item ();
}

void SchedulerMaster::delete_item () NIRVANA_NOEXCEPT
{
	Base::delete_item ();
}

void SchedulerMaster::schedule (DeadlineTime deadline, Executor& executor) NIRVANA_NOEXCEPT
{
	try {
		Base::schedule (deadline, executor);
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
	}
}

bool SchedulerMaster::reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) NIRVANA_NOEXCEPT
{
	if (error_ >= 0)
		return false;

	try {
		if (!Base::reschedule (deadline, executor, old))
			return false;
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
		return false;
	}
	return true;
}

void SchedulerMaster::shutdown () NIRVANA_NOEXCEPT
{
	worker_threads_.shutdown ();
}

void SchedulerMaster::worker_thread_proc () NIRVANA_NOEXCEPT
{
	worker_threads_.thread_proc ();
}

void SchedulerMaster::WorkerThreads::completed (_OVERLAPPED* ovl, uint32_t size, uint32_t error) NIRVANA_NOEXCEPT
{
	Executor* executor = reinterpret_cast <Executor*> (ovl);
	ThreadWorker::execute (*executor, CORBA::Exception::EC_NO_EXCEPTION);
	SchedulerMaster::singleton ().core_free ();
}

}
}
}
