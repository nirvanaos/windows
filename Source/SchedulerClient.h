#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERCLIENT_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERCLIENT_H_

#include "SchedulerIPC.h"
#include "SchedulerAbstract.h"
#include "Mailslot.h"
#include "MailslotReader.h"
#include "WorkerThreads.h"
#include <core.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerClient :
	public SchedulerAbstract,
	private SchedulerIPC,
	public MailslotReader
{
public:
	SchedulerClient (uint64_t protection_domain);

	void run (Runnable& startup, DeadlineTime deadline)
	{
		worker_threads_.run (startup, deadline);
		MailslotReader::terminate ();
	}

	// Implementation of SchedulerAbstract.

	virtual void schedule (DeadlineTime deadline, Executor& executor, DeadlineTime deadline_prev);

	virtual void core_free ();

	virtual void shutdown ()
	{
		// TODO: Send message to the system domain
		worker_threads_.shutdown ();
	}

private:
	virtual void received (OVERLAPPED* ovl, DWORD size);
	void send (const SchedulerMessage& msg);
	
private:
	uint64_t protection_domain_;
	Mailslot scheduler_mailslot_;
	WorkerThreads worker_threads_;
};

inline
SchedulerClient::SchedulerClient (uint64_t protection_domain) :
	protection_domain_ (protection_domain)
{
	DWORD id = GetCurrentProcessId ();
	static const WCHAR prefix [] = EXECUTE_MAILSLOT_PREFIX;
	initialize (prefix, id, sizeof (Execute), worker_threads_);

	try {
		if (!scheduler_mailslot_.open (SCHEDULER_MAILSLOT_NAME))
			throw ::CORBA::INITIALIZE ();

		HANDLE hevent = CreateEventW (nullptr, FALSE, FALSE, nullptr);
		assert (hevent);
		bool success = false;

		try {
			{
				SchedulerMessage msg;
				msg.tag = SchedulerMessage::PROCESS_START;
				msg.msg.process_start.protection_domain = protection_domain;
				msg.msg.process_start.process_id = id;
				send (msg);
			}

			{
				OVERLAPPED ovl;
				memset (&ovl, 0, sizeof (ovl));
				ovl.hEvent = (HANDLE)((LONG_PTR)hevent | 1);

				ProcessStartAck ack;
				if (!ReadFile (handle_, &ack, sizeof (ack), nullptr, &ovl)) {
					DWORD err = GetLastError ();
					if (ERROR_IO_PENDING != err)
						throw ::CORBA::INITIALIZE ();

					if (WAIT_OBJECT_0 != WaitForSingleObject (hevent, SCHEDULER_ACK_TIMEOUT))
						CancelIoEx (handle_, &ovl);
					else
						success = true;

					ovl.hEvent = hevent;
				} else
					success = true;

				DWORD size;
				GetOverlappedResult (handle_, &ovl, &size, TRUE);

				if (success && (sizeof (ack) != size || ack.error != 0))
					success = false;
			}

		} catch (...) {
			CloseHandle (hevent);
			throw;
		}

		CloseHandle (hevent);

		if (!success)
			throw ::CORBA::INITIALIZE ();

	} catch (...) {
		terminate ();
		throw;
	}
}

}
}
}

#endif
