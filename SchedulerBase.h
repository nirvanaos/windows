// Nirvana project
// Windows implementation.
// Scheduler.
// Scheduler is part of the System Domain, system-wide singleton object.

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULER_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULER_H_

#include "SchedulerIPC.h"
#include "../core.h"
#include <ORB.h>
#include <limits>
#include <vector>

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerBase : 
	public SchedulerIPC
{
public:
	SchedulerBase () :
		stop_ (false),
		free_cores_semaphore_ (nullptr),
		scheduler_mailslot_ (INVALID_HANDLE_VALUE),
		completion_port_ (nullptr)
	{
		LONG core_cnt = hardware_concurrency ();
		free_cores_semaphore_ = CreateSemaphoreW (nullptr, core_cnt, core_cnt, free_cores_semaphore_name_);
		if (!free_cores_semaphore_)
			throw ::CORBA::INITIALIZE ();
		queue_semaphore_ = CreateSemaphoreW (nullptr, 0, std::numeric_limits <LONG>::max (), nullptr);
		if (!queue_semaphore_)
			throw ::CORBA::INITIALIZE ();
		scheduler_mailslot_ = create_mailslot (scheduler_mailslot_name_, sizeof (MsgReady));
		completion_port_ = CreateIoCompletionPort (scheduler_mailslot_, nullptr, 1, core_cnt);
		if (!completion_port_)
			throw ::CORBA::INITIALIZE ();

		thread_ = CreateThread (nullptr, 0, thread_proc, this, 0, nullptr);
		assert (thread_);
		verify (SetThreadPriority (thread_, THREAD_PRIORITY_TIME_CRITICAL));
	}

	~SchedulerBase ()
	{
		verify (QueueUserAPC (stop_proc, thread_, (ULONG_PTR)this));
		WaitForSingleObject (thread_, INFINITE);
		CloseHandle (thread_);
		CloseHandle (free_cores_semaphore_);
		CloseHandle (scheduler_mailslot_);
	}

private:
	void close ();
	static DWORD WINAPI thread_proc (void* _this);
	static void CALLBACK read_complete (DWORD error_code, DWORD size, OVERLAPPED* ovl);
	static void CALLBACK stop_proc (ULONG_PTR _this);
	void run ();
	void receive_message (DWORD size);
	void run_worker_thread ();

	struct PerCore
	{
		MsgReady buffer;
		RandomGen random_gen;
		HANDLE thread;
		bool buffer_enqueued;
	};

private:
	HANDLE free_cores_semaphore_;
	HANDLE queue_semaphore_;
	HANDLE scheduler_mailslot_;
	HANDLE completion_port_;
	std::vector <PerCore, CoreAllocator <PerCore> > buffers_;
	bool stop_;
};

}
}
}

#endif
