// Nirvana project
// Windows implementation.
// Scheduler.
// Scheduler is part of the System Domain, system-wide singleton object.

#ifndef NIRVANA_CORE_WINDOWS_MEMORYWINDOWS_H_
#define NIRVANA_CORE_WINDOWS_MEMORYWINDOWS_H_

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
};

class Scheduler : private SchedulerData
{
public:
	Scheduler () :
		stop_ (false)
	{
		LONG core_cnt = std::thread::hardware_concurrency ();
		free_cores_semaphore_ = CreateSemaphoreW (nullptr, core_cnt, core_cnt, OBJ_NAME_PREFIX L".free_cores_semaphore");
		if (!free_cores_semaphore_)
			throw ::CORBA::INITIALIZE ();
		mailslot_ = CreateMailslotW (L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\scheduler_mailslot", sizeof (MessageSchedule), 
																 MAILSLOT_WAIT_FOREVER, nullptr);
		if (INVALID_HANDLE_VALUE == mailslot_) {
			CloseHandle (free_cores_semaphore_);
			throw ::CORBA::INITIALIZE ();
		}
	}

	~Scheduler ()
	{
		stop ();
		CloseHandle (free_cores_semaphore_);
		CloseHandle (mailslot_);
	}

private:
	static DWORD WINAPI thread_proc (void* _this);
	static void CALLBACK read_complete (DWORD error_code, DWORD size, OVERLAPPED* ovl);
	static void CALLBACK stop_proc (ULONG_PTR _this);
	void run ();
	void stop ();
	void receive_message ();
	void run_worker_thread ();

	class Process;

	struct DeadlineInfo
	{
		Process& process;
		std::unordered_map <ULONG_PTR, std::map <DeadlineTime, DeadlineInfo>::iterator>::iterator sync_domain;

		DeadlineInfo (Process& pr, std::unordered_map <ULONG_PTR, std::map <DeadlineTime, DeadlineInfo>::iterator>::iterator it) :
			process (pr), 
			sync_domain (it)
		{}
	};

	typedef std::unordered_map <ULONG_PTR, std::map <DeadlineTime, DeadlineInfo>::iterator> ReadyDomains;

	typedef std::map <DeadlineTime, DeadlineInfo> Deadlines;

	class Process
	{
	public:
		Process (DWORD process_id)
		{
			static const WCHAR fmt [] = L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\run_mailslot%08X";
			WCHAR name [_countof (fmt) + 8 - 3];
			wsprintfW (name, fmt, process_id);
			mailslot_ = CreateFileW (name, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (INVALID_HANDLE_VALUE == mailslot_)
				throw ::CORBA::INTERNAL ();
		}
		
		~Process ()
		{
			CloseHandle (mailslot_);
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
		
	private:
		HANDLE mailslot_;
		ReadyDomains ready_domains_;
	};

private:
	HANDLE free_cores_semaphore_;
	HANDLE mailslot_;
	MessageSchedule message_;
	typedef std::map <DWORD, Process> Processes;
	Processes processes_;
	Deadlines deadlines_;
	bool stop_;
};

}
}
}

#endif
