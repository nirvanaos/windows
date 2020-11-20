// Nirvana project.
// Windows implementation.
// MailslotReader class.

#ifndef NIRVANA_CORE_WINDOWS_MAILSLOTREADER_H_
#define NIRVANA_CORE_WINDOWS_MAILSLOTREADER_H_

#include "BufferPool.h"
#include "CompletionPort.h"
#include <Nirvana/real_copy.h>

namespace Nirvana {
namespace Core {
namespace Windows {

/// Derived class must override `virtual void CompletionPortReceiver::received()` method to process data.
/// Overridden method must get data pointer by call data(ovl), read the data
/// and immediatelly call `MailslotReader::enqueue_buffer()` method.
class MailslotReader :
	public CompletionPortReceiver,
	public BufferPool
{
public:
	/// Create mailslot.
	/// \param mailslot_name Name of the mailslot for incoming messages.
	/// \returns `true` on success. `false` if mailslot already exists.
	/// \throws CORBA::INITIALIZE
	bool create_mailslot (LPCWSTR mailslot_name);

	HANDLE mailslot_handle () const
	{
		return handle_;
	}

protected:
	MailslotReader (size_t buffer_count, DWORD max_msg_size);
	~MailslotReader ();

	/// Put the reader to work.
	void start (CompletionPort& port);

	/// Stop all work.
	void terminate () NIRVANA_NOEXCEPT;

	/// Enqueue read request.
	void enqueue_buffer (OVERLAPPED* ovl) NIRVANA_NOEXCEPT
	{
		if (!ReadFile (handle_, data (ovl), (DWORD)buffer_size (), nullptr, ovl))
			assert (ERROR_IO_PENDING == GetLastError ());
	}

private:
	void cancel_buffer (OVERLAPPED* ovl)
	{
		if (CancelIoEx (handle_, ovl)) {
			DWORD cb;
			GetOverlappedResult (handle_, ovl, &cb, TRUE);
		}
	}

private:
	HANDLE handle_;
	DWORD max_msg_size_;
};

}
}
}

#endif
