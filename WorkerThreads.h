// Nirvana project.
// Windows implementation.
// Worker threads.

#ifndef NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_
#define NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_

#include "SchedulerIPC.h"
#include "../core.h"
#include "../ThreadWorker.h"
#include <ORB.h>
#include <vector>

namespace Nirvana {
namespace Core {

class SyncDomain;

namespace Windows {

class WorkerThreads :
	private SchedulerIPC
{
public:
	WorkerThreads () :
		free_cores_semaphore_ (nullptr),
		scheduler_mailslot_ (INVALID_HANDLE_VALUE),
		run_mailslot_ (INVALID_HANDLE_VALUE),
		completion_port_ (nullptr),
		terminate_ (false)
	{
		free_cores_semaphore_ = OpenSemaphoreW (SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, free_cores_semaphore_name_);
		if (!free_cores_semaphore_)
			throw ::CORBA::INITIALIZE ();
		try {
			scheduler_mailslot_ = open_mailslot (scheduler_mailslot_name_);
			
			DWORD id = GetCurrentProcessId ();
			unsigned thread_cnt = hardware_concurrency ();

			run_mailslot_ = open_run_mailslot (id, true);

			completion_port_ = CreateIoCompletionPort (run_mailslot_, nullptr, 1, thread_cnt);
			if (!completion_port_)
				throw ::CORBA::INITIALIZE ();

			buffers_.resize (thread_cnt);

			for (auto p = buffers_.begin (); p != buffers_.end (); ++p)
				enqueue_buffer (*p);

			threads_.resize (thread_cnt);

			MsgProcessStart msg = {id, 0};
			if (!send (scheduler_mailslot_, msg))
				throw ::CORBA::INITIALIZE ();

		} catch (...) {
			close ();
			throw;
		}
	}

	~WorkerThreads ()
	{
		MsgProcessStop msg = {GetCurrentProcessId ()};
		send (scheduler_mailslot_, msg);
		close ();
	}

	void schedule (DeadlineTime deadline, void* sync_domain, bool update)
	{
		MsgSyncDomainReady msg;
		msg.deadline = deadline;
		msg.process_id = GetCurrentProcessId ();
		msg.sync_domain = (ULONG_PTR)sync_domain;
		msg.update = update;

		if (!send (scheduler_mailslot_, msg))
			throw ::CORBA::INTERNAL ();
	}

private:
	friend class ThreadWorkerBase;

	struct Buffer :
		public OVERLAPPED
	{
		MsgRun message;
		bool enqueued;

		Buffer () :
			enqueued (false)
		{}
	};

	void enqueue_buffer (Buffer& buf)
	{
		if (ReadFile (run_mailslot_, &buf.message, sizeof (buf.message), nullptr, &buf))
			buf.enqueued = true;
	}

	Buffer* get_message ()
	{
		DWORD cb;
		ULONG_PTR key;
		OVERLAPPED* ovl;
		if (GetQueuedCompletionStatus (completion_port_, &cb, &key, &ovl, INFINITE)) {
			assert (cb == sizeof (MsgRun));
			return static_cast <Buffer*> (ovl);
		} else {
			assert (ERROR_ABANDONED_WAIT_0 == GetLastError ());
		}
		return nullptr;
	}

	void worker_thread_proc (ThreadWorker& thread)
	{
		while (Buffer* buf = get_message ()) {
			SyncDomain* sync_domain = (SyncDomain*)buf->message.sync_domain;
			enqueue_buffer (*buf);
			thread.execute (*sync_domain);
			ReleaseSemaphore (free_cores_semaphore_, 1, nullptr);
		}
	}

	void close ();

private:
	HANDLE scheduler_mailslot_;
	HANDLE run_mailslot_;
	HANDLE free_cores_semaphore_;
	HANDLE completion_port_;
	std::vector <Buffer, CoreAllocator <Buffer> > buffers_;
	std::vector <ThreadWorker, CoreAllocator <ThreadWorker> > threads_;
	bool terminate_;
};

}
}
}

#endif
