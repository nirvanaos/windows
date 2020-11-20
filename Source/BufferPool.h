// Nirvana project.
// Windows implementation.
// BufferPool class.

#ifndef NIRVANA_CORE_WINDOWS_BUFFERPOOL_H_
#define NIRVANA_CORE_WINDOWS_BUFFERPOOL_H_

#include <core.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class BufferPool
{
private:
	/// Deprecated
	BufferPool (const BufferPool&) = delete;
	/// Deprecated
	BufferPool& operator = (const BufferPool&) = delete;

public:
	BufferPool (size_t buffer_count, size_t buffer_size) NIRVANA_NOEXCEPT;
	~BufferPool () NIRVANA_NOEXCEPT;

	/// Returns data buffer pointer for specified OVERLAPPED pointer.
	static void* data (OVERLAPPED* ovl) NIRVANA_NOEXCEPT
	{
		return ovl + 1;
	}

protected:
	size_t buffer_size () const NIRVANA_NOEXCEPT
	{
		return buffer_size_;
	}

	OVERLAPPED* begin () const NIRVANA_NOEXCEPT
	{
		return begin_;
	}

	OVERLAPPED* end () const NIRVANA_NOEXCEPT
	{
		return end_;
	}

	OVERLAPPED* next (OVERLAPPED* ovl) const NIRVANA_NOEXCEPT
	{
		return (OVERLAPPED*)(((BYTE*)(ovl + 1)) + buffer_size_);
	}

private:
	OVERLAPPED* begin_;
	OVERLAPPED* end_;
	size_t buffer_size_;
};

}
}
}

#endif
