// Nirvana project.
// Windows implementation.
// CompletionPort class.

#ifndef NIRVANA_CORE_WINDOWS_COMPLETIONPORT_H_
#define NIRVANA_CORE_WINDOWS_COMPLETIONPORT_H_

#include "CompletionPortReceiver.h"
#include "../Port/SystemInfo.h"

namespace Nirvana {
namespace Core {
namespace Windows {

/// Windows I/O completion port wrapper.
class NIRVANA_NOVTABLE CompletionPort
{
private:
	CompletionPort (const CompletionPort&) = delete;
	CompletionPort& operator = (const CompletionPort&) = delete;

public:
	static unsigned int thread_count ()
	{
		return Port::SystemInfo::hardware_concurrency ();
	}

	CompletionPort ();

	~CompletionPort ()
	{
		if (completion_port_)
			CloseHandle (completion_port_);
	}

	/// Add a CompletionPortReceiver object listening this completion port.
	void add_receiver (HANDLE hf, CompletionPortReceiver& receiver)
	{
		create (hf, &receiver);
	}

	/// Post an I/O completion packet to an I/O completion port.
	void post (CompletionPortReceiver& receiver, OVERLAPPED* param, DWORD size) NIRVANA_NOEXCEPT
	{
		verify (PostQueuedCompletionStatus (completion_port_, size, (ULONG_PTR)&receiver, param));
	}

	/// On close completion port all threads will return with `ERROR_ABANDONED_WAIT_0` error code.
	virtual void terminate () NIRVANA_NOEXCEPT
	{
		HANDLE port = completion_port_;
		completion_port_ = nullptr;
		if (port)
			CloseHandle (port);
	}

	/// Worker thread procedure.
	void thread_proc () NIRVANA_NOEXCEPT;

protected:
	/// Ensure that port exists.
	void start ()
	{
		if (!completion_port_)
			create (INVALID_HANDLE_VALUE, nullptr);
	}

private:
	void create (HANDLE hfile, CompletionPortReceiver* receiver);

private:
	HANDLE completion_port_;
};

}
}
}

#endif
