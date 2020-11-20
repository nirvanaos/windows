// Nirvana project.
// Windows implementation.
// BufferPool class.

#include <Heap.h>
#include "BufferPool.h"

namespace Nirvana {
namespace Core {
namespace Windows {

BufferPool::BufferPool (size_t buffer_count, size_t buffer_size) NIRVANA_NOEXCEPT :
begin_ (nullptr),
end_ (nullptr),
buffer_size_ (round_up (buffer_size, sizeof (LONG_PTR)))
{
	size_t size = buffer_count * (sizeof (OVERLAPPED) + buffer_size_);
	begin_ = (OVERLAPPED*)g_core_heap.allocate (nullptr, size, Memory::ZERO_INIT);
	end_ = (OVERLAPPED*)(((BYTE*)begin_) + size);
}

BufferPool::~BufferPool () NIRVANA_NOEXCEPT
{
	g_core_heap.release (begin_, (BYTE*)end_ - (BYTE*)begin_);
}

}
}
}
