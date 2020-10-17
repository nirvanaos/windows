// Nirvana project.
// Windows implementation.
// BufferPool class.

#include "BufferPool.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void BufferPool::initialize (CompletionPort& port, DWORD buffer_size)
{
	port.add_receiver (handle_, *this);

	size_t buffer_count = port.thread_count ();
	buffer_size_ = round_up (buffer_size, sizeof (LONG_PTR));
	size_t size = buffer_count * (sizeof (OVERLAPPED) + buffer_size);
	begin_ = (OVERLAPPED*)g_core_heap.allocate (nullptr, size, 0);
	end_ = (OVERLAPPED*)(((BYTE*)begin_) + size);
	for (OVERLAPPED* p = begin (); p != end (); p = next (p)) {
		memset (p, 0, sizeof (OVERLAPPED));
	}
	for (OVERLAPPED* p = begin (); p != end (); p = next (p)) {
		enqueue_buffer (p);
	}
}

void BufferPool::terminate ()
{
	g_core_heap.release (begin_, (BYTE*)end_ - (BYTE*)begin_);
	begin_ = end_ = nullptr;
}

}
}
}
