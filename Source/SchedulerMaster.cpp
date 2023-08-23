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
#include "app_data.h"
#include <StartupSys.h>

//#define DEBUG_SHUTDOWN

#ifdef DEBUG_SHUTDOWN
#include "../Port/Debugger.h"
#endif

namespace Nirvana {
namespace Core {
namespace Windows {

SchedulerMaster::SchedulerMaster () :
	sysdomainid_ (INVALID_HANDLE_VALUE),
	watchdog_ (*this)
{}

SchedulerMaster::~SchedulerMaster ()
{
	if (INVALID_HANDLE_VALUE != sysdomainid_) {
		CloseHandle (sysdomainid_);
		sysdomainid_ = INVALID_HANDLE_VALUE;
	}
	Office::terminate ();
	worker_threads_.terminate ();
	watchdog_.terminate ();
}

void SchedulerMaster::create_folders ()
{
	WCHAR path [MAX_PATH + 1];
	size_t cc = get_app_data_path (path, std::size (path), true);
	if (!cc)
		throw_INITIALIZE ();
	WCHAR* root_end = path + cc;

	static const WCHAR* const folders [] = {
		WINWCS ("var\\log"),
		WINWCS ("etc")
	};

	for (const WCHAR* const* p = folders; p != std::end (folders); ++p) {
		if (!create_app_data_folder (path, root_end, *p))
			throw_INITIALIZE ();
	}
}

bool SchedulerMaster::run (StartupSys& startup)
{
	sys_process_id = GetCurrentProcessId ();

	if (!Office::create_mailslot (SCHEDULER_MAILSLOT_NAME))
		return false; // System domain is already running

	create_folders ();

	if (!watchdog_.start ())
		throw_INITIALIZE ();

	Office::start ();

	sysdomainid_ = open_sysdomainid (true);
	if (INVALID_HANDLE_VALUE == sysdomainid_)
		throw_INITIALIZE ();

	DWORD written;
	if (!WriteFile (sysdomainid_, &sys_process_id, sizeof (DWORD), &written, nullptr))
		throw_INITIALIZE ();

	worker_threads_.run (startup, INFINITE_DEADLINE);

	return true;
}

void SchedulerMaster::create_item ()
{
	Base::create_item ();
}

void SchedulerMaster::delete_item () noexcept
{
	Base::delete_item ();
}

void SchedulerMaster::schedule (DeadlineTime deadline, Executor& executor) noexcept
{
	try {
		Base::schedule (deadline, executor);
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
	}
}

bool SchedulerMaster::reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) noexcept
{
	try {
		if (Base::reschedule (deadline, executor, old))
			return true;
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
	}
	return false;
}

void SchedulerMaster::shutdown () noexcept
{
#ifdef DEBUG_SHUTDOWN
	Port::Debugger::output_debug_string ("Shutdown 2\n");
#endif
	Office::terminate ();
	worker_threads_.shutdown ();
}

void SchedulerMaster::worker_thread_proc () noexcept
{
	worker_threads_.thread_proc ();
}

void SchedulerMaster::WorkerThreads::completed (_OVERLAPPED* ovl, uint32_t size, uint32_t error) noexcept
{
	Executor* executor = reinterpret_cast <Executor*> (ovl);
	ThreadWorker::execute (*executor);
	SchedulerMaster::singleton ().core_free ();
}

}
}
}
