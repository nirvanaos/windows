#include "PostOffice.h"
#include "SystemInfo.h"
#include <ORB.h>

namespace Nirvana {
namespace Core {
namespace Windows {

bool PostOfficeBase::initialize (LPCWSTR mailslot_name)
{
	assert (!running_);
	mailslot_ = CreateMailslotW (mailslot_name, max_msg_size_, MAILSLOT_WAIT_FOREVER, nullptr);
	if (INVALID_HANDLE_VALUE == mailslot_) {
		DWORD err = GetLastError ();
		if (ERROR_ALREADY_EXISTS == err)
			return false;
		throw ::CORBA::INITIALIZE ();
	}

	running_ = true;
	thread_count_ = SystemInfo::hardware_concurrency ();
	completion_port_ = CreateIoCompletionPort (mailslot_, nullptr, 1, thread_count_);
	if (!completion_port_)
		throw ::CORBA::INITIALIZE ();
	return true;
}

void PostOfficeBase::close_handles ()
{
	if (completion_port_) {
		CloseHandle (completion_port_);
		completion_port_ = nullptr;
	}
	if (INVALID_HANDLE_VALUE != mailslot_) {
		CloseHandle (mailslot_);
		mailslot_ = INVALID_HANDLE_VALUE;
	}
}

PostOfficeBase::Buffer* PostOfficeBase::get_message ()
{
	if (running_) {
		ULONG_PTR key;
		OVERLAPPED* ovl;
		DWORD size;
		if (GetQueuedCompletionStatus (completion_port_, &size, &key, &ovl, INFINITE)) {
			Buffer* buffer = static_cast <Buffer*> (ovl);
			buffer->enqueued_ = 0;
			buffer->size_ = size;
			return buffer;
		} else {
			DWORD err = GetLastError ();
			switch (err) {
			case ERROR_ABANDONED_WAIT_0:
			case ERROR_OPERATION_ABORTED:
				break;

			default:
				throw ::CORBA::INTERNAL ();
			}
		}
	}
	return nullptr;
}

inline void PostOfficeBase::Buffer::enqueue (HANDLE h, DWORD size)
{
	enqueued_ = true;
	if (!ReadFile (h, const_cast <void*> (message ()), size, nullptr, this)) {
		DWORD err = GetLastError ();
		if (ERROR_IO_PENDING != err) {
			enqueued_ = false;
			throw ::CORBA::INTERNAL ();
		}
	}
}

void PostOfficeBase::enqueue_buffer (Buffer& buffer)
{
	buffer.enqueue (mailslot_, max_msg_size_);
}

}
}
}
