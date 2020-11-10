// Nirvana project.
// Windows implementation.
// ThreadPool template class.

#include "CompletionPort.h"
#include <Nirvana/throw_exception.h>

namespace Nirvana {
namespace Core {
namespace Windows {

CompletionPort::CompletionPort () :
	completion_port_ (nullptr)
{}

void CompletionPort::create (HANDLE hfile, CompletionPortReceiver* receiver)
{
	HANDLE port = CreateIoCompletionPort (hfile, completion_port_, (ULONG_PTR)receiver, thread_count ());
	if (!port)
		throw_INITIALIZE ();
	assert (completion_port_ == nullptr || completion_port_ == port);
	completion_port_ = port;
}

inline bool CompletionPort::dispatch ()
{
	if (completion_port_) {
		ULONG_PTR key;
		OVERLAPPED* ovl;
		DWORD size;
		if (GetQueuedCompletionStatus (completion_port_, &size, &key, &ovl, INFINITE)) {
			reinterpret_cast <CompletionPortReceiver*> (key)->received (ovl, size);
			return true;
		} else {
			DWORD err = GetLastError ();
			switch (err) {
			case ERROR_OPERATION_ABORTED:
			case ERROR_ABANDONED_WAIT_0:
				break;

			default:
				throw_INTERNAL ();
			}
		}
	}
	return false;
}

void CompletionPort::thread_proc ()
{
	while (dispatch ());
}

}
}
}
