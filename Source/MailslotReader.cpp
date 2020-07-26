// Nirvana project.
// Windows implementation.
// MailslotReader class.

#include "MailslotReader.h"

namespace Nirvana {
namespace Core {
namespace Windows {

bool MailslotReader::initialize (LPCWSTR mailslot_name, DWORD max_msg_size, CompletionPort& port)
{
	handle_ = CreateMailslotW (mailslot_name, max_msg_size, MAILSLOT_WAIT_FOREVER, nullptr);
	if (INVALID_HANDLE_VALUE == handle_) {
		DWORD err = GetLastError ();
		if (ERROR_ALREADY_EXISTS == err)
			return false;
		throw ::CORBA::INITIALIZE ();
	}
	BufferPool::initialize (port, max_msg_size);
	return true;
}

void MailslotReader::enqueue_buffer (OVERLAPPED* ovl)
{
	if (!ReadFile (handle_, data (ovl), (DWORD)buffer_size (), nullptr, ovl)) {
		DWORD err = GetLastError ();
		if (ERROR_IO_PENDING != err)
			throw ::CORBA::INTERNAL ();
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
