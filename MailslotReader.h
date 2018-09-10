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

	bool start (LPCWSTR mailslot_name, DWORD max_msg_size, CompletionPort& port);

	template <size_t PREFIX_SIZE>
	void start (const WCHAR (&prefix) [PREFIX_SIZE], DWORD id, size_t max_msg_size, CompletionPort& port)
	{
		WCHAR name [PREFIX_SIZE + 8];
		_ultow (id, std::copy (prefix, prefix + PREFIX_SIZE - 1, name), 16);
		if (!start (name, max_msg_size, port))
			throw ::CORBA::INITIALIZE ();
	}

	virtual void terminate ();
	virtual void enqueue_buffer (OVERLAPPED* ovl);

private:
	void cancel_buffer (OVERLAPPED* ovl)
	{
		CancelIoEx (handle_, ovl);
	}
};

}
}
}

#endif
