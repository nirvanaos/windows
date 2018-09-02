// Nirvana project
// Windows implementation.
// Scheduler.
// Scheduler is part of the System Domain, system-wide singleton object.

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULER_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULER_H_

#include "SchedulerIPC.h"
#include "../core.h"
#include <map>
#include <unordered_map>

namespace Nirvana {
namespace Core {
namespace Windows {

class Scheduler : 
	public SchedulerIPC
{
public:
	Scheduler () :
		stop_ (false)
	{
		LONG core_cnt = hardware_concurrency ();
		free_cores_semaphore_ = CreateSemaphoreW (nullptr, core_cnt, core_cnt, free_cores_semaphore_name_);
		if (!free_cores_semaphore_)
			throw ::CORBA::INITIALIZE ();
		try {
			scheduler_mailslot_ = create_mailslot (scheduler_mailslot_name_, sizeof (SchedulerMessage));
		} catch (...) {
			CloseHandle (free_cores_semaphore_);
			throw;
		}

		thread_ = CreateThread (nullptr, 0, thread_proc, this, 0, nullptr);
		assert (thread_);
		verify (SetThreadPriority (thread_, THREAD_PRIORITY_TIME_CRITICAL));
	}

	~Scheduler ()
	{
		verify (QueueUserAPC (stop_proc, thread_, (ULONG_PTR)this));
		WaitForSingleObject (thread_, INFINITE);
		CloseHandle (thread_);
		CloseHandle (free_cores_semaphore_);
		CloseHandle (scheduler_mailslot_);
	}

private:
	static DWORD WINAPI thread_proc (void* _this);
	static void CALLBACK read_complete (DWORD error_code, DWORD size, OVERLAPPED* ovl);
	static void CALLBACK stop_proc (ULONG_PTR _this);
	void run ();
	void receive_message (DWORD size);
	void run_worker_thread ();

	class Process;
	struct DeadlineInfo;

	typedef std::map <DeadlineTime, DeadlineInfo,
		std::less <DeadlineTime>,
		CoreAllocator <std::pair <DeadlineTime, DeadlineInfo> >
	> Deadlines;

	typedef std::unordered_map <ULONG_PTR, Deadlines::iterator,
		std::hash <ULONG_PTR>, std::equal_to <ULONG_PTR>,
		CoreAllocator <std::pair <ULONG_PTR, Deadlines::iterator> >
	> ReadyDomains;

	struct DeadlineInfo
	{
		Process& process;
		ReadyDomains::iterator sync_domain;

		DeadlineInfo (Process& p, ReadyDomains::iterator d) :
			process (p), 
			sync_domain (d)
		{}
	};

	class Process
	{
	public:
		Process (DWORD process_id)
		{
			run_mailslot_ = open_run_mailslot (process_id, false);
		}
		
		~Process ()
		{
			CloseHandle (run_mailslot_);
		}
		
		void schedule (Deadlines& deadlines, const MsgSyncDomainReady& msg)
		{
			if (msg.update) {
				ReadyDomains::iterator domain = ready_domains_.find (msg.sync_domain);
				if (domain != ready_domains_.end ()) {
					deadlines.erase (domain->second);
					domain->second = deadlines.emplace (msg.deadline, DeadlineInfo (*this, domain)).first;
				}
			} else {
				auto ins = ready_domains_.emplace (msg.sync_domain, Deadlines::iterator ());
				if (!ins.second)
					deadlines.erase (ins.first->second);
				ins.first->second = deadlines.emplace (msg.deadline, DeadlineInfo (*this, ins.first)).first;
			}
		}

		void run (ReadyDomains::iterator domain)
		{
			MsgRun msg = {domain->first};
			ready_domains_.erase (domain);
			DWORD cb;
			WriteFile (run_mailslot_, &msg, sizeof (msg), &cb, nullptr);
		}

		void clear (Deadlines& deadlines)
		{
			for (auto p = ready_domains_.begin (); p != ready_domains_.end (); ++p)
				deadlines.erase (p->second);
			ready_domains_.clear ();
		}
		
	private:
		HANDLE run_mailslot_;
		ReadyDomains ready_domains_;
	};

private:
	typedef std::unordered_map <DWORD, Process,
		std::hash <DWORD>, std::equal_to <DWORD>,
		CoreAllocator <std::pair <DWORD, Process> >
	> Processes;

	HANDLE free_cores_semaphore_;
	HANDLE scheduler_mailslot_;
	SchedulerMessage message_;
	Processes processes_;
	Deadlines deadlines_;
	HANDLE thread_;
	bool stop_;
};

}
}
}

#endif
