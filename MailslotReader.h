// Nirvana project.
// Windows implementation.
// MailslotReader class.

#ifndef NIRVANA_CORE_WINDOWS_MAILSLOTREADER_H_
#define NIRVANA_CORE_WINDOWS_MAILSLOTREADER_H_

#include "BufferPool.h"
#include <algorithm>

namespace Nirvana {
namespace Core {
namespace Windows {

class MailslotReader :
	public BufferPool
{
protected:
	~MailslotReader ()
	{
		terminate ();
	}

	/// Put the reader to work.
	/// \param mailslot_name Name of the mailslot for incoming messages.
	/// \returns `true` on success. `false` if mailslot already exists.
	/// If exception occurs, terminate() will be called internally.
	bool initialize (LPCWSTR mailslot_name, DWORD max_msg_size, CompletionPort& port);

	/// Put the reader to work.
	/// \param prefix Mailslot name prefix.
	/// \id Unique id of the mailslot. Usually it is id of the current process.
	/// If exception occurs, terminate() will be called internally.
	template <size_t PREFIX_SIZE>
	void initialize (const WCHAR (&prefix) [PREFIX_SIZE], DWORD id, size_t max_msg_size, CompletionPort& port)
	{
		WCHAR name [PREFIX_SIZE + 8];
		_ultow (id, std::copy (prefix, prefix + PREFIX_SIZE - 1, name), 16);
		if (!initialize (name, max_msg_size, port))
			throw ::CORBA::INITIALIZE ();
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
