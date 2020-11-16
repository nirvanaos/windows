// Nirvana project.
// Windows implementation.
// MailslotReader class.

#include "MailslotReader.h"

namespace Nirvana {
namespace Core {
namespace Windows {

bool MailslotReader::create_mailslot (LPCWSTR mailslot_name, size_t max_msg_size)
{
	assert (INVALID_HANDLE_VALUE == handle_);
	handle_ = CreateMailslotW (mailslot_name, (DWORD)max_msg_size, MAILSLOT_WAIT_FOREVER, nullptr);
	if (INVALID_HANDLE_VALUE == handle_) {
		DWORD err = GetLastError ();
		if (ERROR_ALREADY_EXISTS == err)
			return false;
		throw_INITIALIZE ();
	}
	return true;
}

void MailslotReader::enqueue_buffer (OVERLAPPED* ovl)
{
	if (!ReadFile (handle_, data (ovl), (DWORD)buffer_size (), nullptr, ovl)) {
		DWORD err = GetLastError ();
		if (ERROR_IO_PENDING != err)
			throw_INTERNAL ();
	}
}

void MailslotReader::terminate ()
{
	if (INVALID_HANDLE_VALUE != handle_) {
		for (OVERLAPPED* p = begin (); p != end (); p = next (p)) {
			cancel_buffer (p);
		}
		CloseHandle (handle_);
		handle_ = INVALID_HANDLE_VALUE;
	}
	BufferPool::terminate ();
}

}
}
}
