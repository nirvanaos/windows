// Nirvana project.
// Windows implementation.
// MailslotReader class.

#include "MailslotReader.h"

namespace Nirvana {
namespace Core {
namespace Windows {

MailslotReader::MailslotReader (size_t buffer_count, DWORD max_msg_size) :
	BufferPool (buffer_count, max_msg_size),
	max_msg_size_ (max_msg_size),
	handle_ (INVALID_HANDLE_VALUE)
{}

MailslotReader::~MailslotReader ()
{
	assert (INVALID_HANDLE_VALUE == handle_); // terminate() must be called in all cases
}

bool MailslotReader::create_mailslot (LPCWSTR mailslot_name)
{
	assert (INVALID_HANDLE_VALUE == handle_);
	handle_ = CreateMailslotW (mailslot_name, max_msg_size_, MAILSLOT_WAIT_FOREVER, nullptr);
	if (INVALID_HANDLE_VALUE == handle_) {
		DWORD err = GetLastError ();
		if (ERROR_ALREADY_EXISTS == err)
			return false;
		throw_INITIALIZE ();
	}
	return true;
}

void MailslotReader::start (CompletionPort& port)
{
	assert (handle_ != INVALID_HANDLE_VALUE);
	port.add_receiver (handle_, *this);

	for (OVERLAPPED* p = begin (); p != end (); p = next (p)) {
		enqueue_buffer (p);
	}
}

void MailslotReader::terminate () NIRVANA_NOEXCEPT
{
	if (INVALID_HANDLE_VALUE != handle_) {
		for (OVERLAPPED* p = begin (); p != end (); p = next (p)) {
			cancel_buffer (p);
		}
		CloseHandle (handle_);
		handle_ = INVALID_HANDLE_VALUE;
	}
}

}
}
}
