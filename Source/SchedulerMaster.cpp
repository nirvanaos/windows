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
#include "app_data.h"
#include <StartupSys.h>

//#define DEBUG_SHUTDOWN

#ifdef DEBUG_SHUTDOWN
#include "../Port/Debugger.h"
#endif

namespace Nirvana {
namespace Core {
namespace Windows {

SchedulerProcess::SchedulerProcess (SchedulerMaster& scheduler, DWORD flags) :
	scheduler_ (scheduler),
	semaphore_ (nullptr),
	pipe_ (INVALID_HANDLE_VALUE),
	process_id_ (0),
	valid_cnt_ (0),
	used_cores_ (0),
	created_items_ (0),
	terminated_ ATOMIC_FLAG_INIT,
	buffers_ (scheduler.thread_count (), sizeof (SchedulerMessage::Buffer))
{
	pipe_ = CreateNamedPipeW (SCHEDULER_PIPE_NAME, PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED | flags,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS,
		PIPE_UNLIMITED_INSTANCES, 0, 0, 0, nullptr);
	if (INVALID_HANDLE_VALUE != pipe_) {
		scheduler.add_receiver (pipe_, *this);
		connect ();
	} else
		throw_INITIALIZE ();
}

SchedulerProcess::~SchedulerProcess ()
{
	if (semaphore_)
		CloseHandle (semaphore_);
	CloseHandle (pipe_);
}

inline
void SchedulerProcess::connect () noexcept
{
	if (!ConnectNamedPipe (pipe_, buffers_.begin ()))
		assert (ERROR_IO_PENDING == GetLastError ());
}

void SchedulerProcess::enqueue_buffer (OVERLAPPED* buf) noexcept
{
	_add_ref ();
	if (!ReadFile (pipe_, BufferPool::data (buf), sizeof (SchedulerMessage::Buffer), nullptr, buf))
		assert (ERROR_IO_PENDING == GetLastError ());
}

inline
bool SchedulerProcess::start () noexcept
{
	assert (!semaphore_);
	if (!GetNamedPipeClientProcessId (pipe_, &process_id_))
		return false;
	semaphore_ = OpenSemaphoreW (SEMAPHORE_MODIFY_STATE, false,
		object_name (SCHEDULER_SEMAPHORE_PREFIX, process_id_));
	if (!semaphore_) {
		DisconnectNamedPipe (pipe_);
		return false;
	}

	valid_cnt_.increment ();

	OVERLAPPED* buf = buffers_.begin ();
	do {
		enqueue_buffer (buf);
		buf = buffers_.next (buf);
	} while (buf != buffers_.end ());
	return true;
}

void SchedulerProcess::terminate () noexcept
{
	HANDLE sem = semaphore_;
	semaphore_ = nullptr;
	CloseHandle (sem);
	CloseHandle (pipe_);
	pipe_ = INVALID_HANDLE_VALUE;
	auto cnt = used_cores_.load ();
	while (cnt--) {
		scheduler_.core_free ();
	}
	cnt = created_items_.load ();
	while (cnt--) {
		scheduler_.delete_item ();
	}
}

inline
bool SchedulerProcess::execute () noexcept
{
	if (valid_cnt_.load () && ReleaseSemaphore (semaphore_, 1, nullptr)) {
		used_cores_.increment ();
		return true;
	}
	return false;
}

inline
void SchedulerProcess::core_free ()
{
	used_cores_.decrement ();
	scheduler_.core_free ();
}

void SchedulerProcess::create_item ()
{
	scheduler_.create_item ();
	created_items_.increment ();
}

void SchedulerProcess::delete_item ()
{
	created_items_.decrement ();
	scheduler_.create_item ();
}

void SchedulerProcess::schedule (DeadlineTime deadline)
{
	scheduler_.schedule (deadline, *this);
}

void SchedulerProcess::reschedule (DeadlineTime deadline, DeadlineTime old)
{
	scheduler_.reschedule (deadline, *this, old);
}

void SchedulerProcess::completed (OVERLAPPED* ovl, uint32_t size, uint32_t error) noexcept
{
	if (ERROR_BROKEN_PIPE == error) {
		if (!terminated_.test_and_set () && !valid_cnt_.decrement_seq ())
			terminate ();
	} else if (!error) {
		if (0 == valid_cnt_.load ()) {
			assert (!size);
			if (start ())
				scheduler_.create_process (this);
			else {
				connect ();
				return; // No _remove_ref here
			}
		} else if (size && valid_cnt_.increment_if_not_zero ()) {

			static const size_t MAX_WORDS = (sizeof (SchedulerMessage::Buffer) + sizeof (LONG_PTR) - 1) / sizeof (LONG_PTR);
			LONG_PTR buf [MAX_WORDS];
			LONG_PTR* msg = (LONG_PTR*)BufferPool::data (ovl);
			real_copy (msg, msg + (size + sizeof (LONG_PTR) - 1) / sizeof (LONG_PTR), buf);

			// Enqueue buffer to reading a next message.
			enqueue_buffer (ovl);

			dispatch_message (buf, size);

			if (!valid_cnt_.decrement_seq ())
				terminate ();
		}
	}

	_remove_ref ();
}

void SchedulerProcess::dispatch_message (const void* msg, size_t size)
{
	switch (size) {
		case sizeof (SchedulerMessage::Tagged) :
			switch (((SchedulerMessage::Tagged*)msg)->tag) {
			case SchedulerMessage::Tagged::CREATE_ITEM:
				create_item ();
				break;
			case SchedulerMessage::Tagged::DELETE_ITEM:
				delete_item ();
				break;
			case SchedulerMessage::Tagged::CORE_FREE:
				core_free ();
				break;

			default:
				assert (false);
			}
			break;

		case sizeof (SchedulerMessage::Schedule) :
			schedule (((SchedulerMessage::Schedule*)msg)->deadline);
			break;

		case sizeof (SchedulerMessage::ReSchedule) :
			reschedule (((SchedulerMessage::ReSchedule*)msg)->deadline,
				((SchedulerMessage::ReSchedule*)msg)->deadline_prev);
			break;

		default:
			assert (false);
	}
}

SchedulerMaster::SchedulerMaster () :
	sysdomainid_ (INVALID_HANDLE_VALUE)
{}

SchedulerMaster::~SchedulerMaster ()
{
	if (INVALID_HANDLE_VALUE != sysdomainid_) {
		CloseHandle (sysdomainid_);
		sysdomainid_ = INVALID_HANDLE_VALUE;
	}
	Pool::terminate ();
	worker_threads_.terminate ();
}

void SchedulerMaster::create_folders ()
{
	WinWChar path [MAX_PATH + 1];
	size_t cc = get_app_data_path (path, std::size (path), true);
	if (!cc)
		throw_INITIALIZE ();
	WinWChar* root_end = path + cc;

	static const WinWChar* const folders [] = {
		WINWCS ("var\\log"),
		WINWCS ("etc")
	};

	for (const WinWChar* const* p = folders; p != std::end (folders); ++p) {
		if (!create_app_data_folder (path, root_end, *p))
			throw_INITIALIZE ();
	}
}

bool SchedulerMaster::run (StartupSys& startup)
{
	sys_process_id = GetCurrentProcessId ();

	sysdomainid_ = open_sysdomainid (true);
	if (INVALID_HANDLE_VALUE == sysdomainid_)
		return false; // System domain is already running

	try {
		DWORD written;
		if (!WriteFile (sysdomainid_, &sys_process_id, sizeof (DWORD), &written, nullptr))
			throw_INITIALIZE ();

		create_folders ();

		if (!create_process (nullptr))
			throw_INITIALIZE ();

		Pool::start (SCHEDULER_THREAD_PRIORITY);
		worker_threads_.run (startup, INFINITE_DEADLINE);

	} catch (...) {
		CloseHandle (sysdomainid_);
		sysdomainid_ = INVALID_HANDLE_VALUE;
		throw;
	}
	return true;
}

bool SchedulerMaster::create_process (SchedulerProcess* started) noexcept
{
	try {
		new SchedulerProcess (*this, started ? 0 : FILE_FLAG_FIRST_PIPE_INSTANCE);
	} catch (...) {
		assert (false);
		return false;
	}
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

inline
void SchedulerMaster::schedule (DeadlineTime deadline, SchedulerProcess& process) noexcept
{
	try {
		Base::schedule (deadline, process);
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
	}
}

inline
void SchedulerMaster::reschedule (DeadlineTime deadline, SchedulerProcess& process, DeadlineTime old) noexcept
{
	try {
		Base::reschedule (deadline, process, old);
	} catch (...) {
		on_error (CORBA::SystemException::EC_NO_MEMORY);
	}
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
