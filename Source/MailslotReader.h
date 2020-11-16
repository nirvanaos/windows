// Nirvana project.
// Windows implementation.
// MailslotReader class.

#ifndef NIRVANA_CORE_WINDOWS_MAILSLOTREADER_H_
#define NIRVANA_CORE_WINDOWS_MAILSLOTREADER_H_

#include "BufferPool.h"
#include <Nirvana/real_copy.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class MailslotReader :
	public BufferPool
{
public:
	/// Create mailslot.
	/// \param mailslot_name Name of the mailslot for incoming messages.
	/// \returns `true` on success. `false` if mailslot already exists.
	/// \throws CORBA::INITIALIZE
	bool create_mailslot (LPCWSTR mailslot_name, size_t max_msg_size);

protected:
	~MailslotReader ()
	{
		terminate ();
	}

	/// Put the reader to work.
	/// If exception occurs, terminate() will be called internally.
	void start (CompletionPort& port, size_t buffer_count, size_t max_msg_size)
	{
		BufferPool::start (port, buffer_count, max_msg_size);
	}

	/// Stop all work.
	virtual void terminate ();

	/// Enqueue read request.
	virtual void enqueue_buffer (OVERLAPPED* ovl);

private:
	void cancel_buffer (OVERLAPPED* ovl)
	{
		if (CancelIoEx (handle_, ovl)) {
			DWORD cb;
			GetOverlappedResult (handle_, ovl, &cb, TRUE);
		}
	}
};

}
}
}

#endif
