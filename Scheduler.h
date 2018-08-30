// Nirvana project
// Windows implementation.
// Scheduler.
// Scheduler is part of the System Domain, system-wide singleton object.

#ifndef NIRVANA_CORE_WINDOWS_SCHEDULER_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULER_H_

#include <Nirvana.h>
#include <ORB.h>
#include "win32.h"
#include <map>
#include <unordered_map>
#include <thread>

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerData
{
protected:
	struct ProcessStart
	{
		DWORD process_id;
	};

	struct SyncDomainReady
	{
		DeadlineTime deadline;
		ULONG_PTR sync_domain;
		DWORD process_id;
		BOOL update;
	};
	
	struct ProcessStop
	{
		DWORD process_id;
	};

	struct MessageSchedule
	{
		enum
		{
			SYNC_DOMAIN_READY = 0,
			PROCESS_START = 1,
			PROCESS_STOP = 2
		}
		tag;

		union Body
		{
			SyncDomainReady sync_domain_ready;
			ProcessStart process_start;
			ProcessStop process_stop;
		}
		body;
	};

	struct MessageRun
	{
		ULONG_PTR sync_domain;
	};

	static HANDLE open_run_mailslot (DWORD process_id, bool create);

	static WCHAR scheduler_mailslot_name_ [];
};

class Scheduler : 
	private SchedulerData
{
public:
	static unsigned int hardware_concurrency ()
	{
		::SYSTEM_INFO si;
		::GetSystemInfo (&si);
		return si.dwNumberOfProcessors;
	}

	Scheduler () :
		stop_ (false)
	{
		LONG core_cnt = hardware_concurrency ();
		free_cores_semaphore_ = CreateSemaphoreW (nullptr, core_cnt, core_cnt, OBJ_NAME_PREFIX L".free_cores_semaphore");
		if (!free_cores_semaphore_)
			throw ::CORBA::INITIALIZE ();
		scheduler_mailslot_ = CreateMailslotW (scheduler_mailslot_name_, sizeof (MessageSchedule),
																 MAILSLOT_WAIT_FOREVER, nullptr);
		if (INVALID_HANDLE_VALUE == scheduler_mailslot_) {
			CloseHandle (free_cores_semaphore_);
			throw ::CORBA::INITIALIZE ();
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
	void receive_message ();
	void run_worker_thread ();

	class Process;
	struct DeadlineInfo;

	typedef std::map <DeadlineTime, DeadlineInfo> Deadlines;
	typedef std::unordered_map <ULONG_PTR, Deadlines::iterator> ReadyDomains;

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
		
		void schedule (Deadlines& deadlines, const SyncDomainReady& msg)
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
			MessageRun msg = {domain->first};
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
	HANDLE free_cores_semaphore_;
	HANDLE scheduler_mailslot_;
	MessageSchedule message_;
	typedef std::map <DWORD, Process> Processes;
	Processes processes_;
	Deadlines deadlines_;
	HANDLE thread_;
	bool stop_;
};

}
}
}

#endif
