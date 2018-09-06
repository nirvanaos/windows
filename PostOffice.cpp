#include "PostOffice.h"
#include "SystemInfo.h"

namespace Nirvana {
namespace Core {
namespace Windows {

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
			assert (ERROR_ABANDONED_WAIT_0 == GetLastError ());
		}
	}
	return nullptr;
}

void PostOfficeBase::initialize (LPCWSTR mailslot_name)
{
	running_ = true;
	mailslot_ = CreateMailslotW (mailslot_name, max_msg_size_, MAILSLOT_WAIT_FOREVER, nullptr);
	if (INVALID_HANDLE_VALUE == mailslot_)
		throw ::CORBA::INITIALIZE ();

	thread_count_ = SystemInfo::hardware_concurrency ();
	completion_port_ = CreateIoCompletionPort (mailslot_, nullptr, 1, thread_count_);
	if (!completion_port_)
		throw ::CORBA::INITIALIZE ();
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

}
}
}
