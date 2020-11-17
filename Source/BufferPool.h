// Nirvana project.
// Windows implementation.
// BufferPool class.

#ifndef NIRVANA_CORE_WINDOWS_BUFFERPOOL_H_
#define NIRVANA_CORE_WINDOWS_BUFFERPOOL_H_

#include "CompletionPort.h"

namespace Nirvana {
namespace Core {
namespace Windows {

/// Derived class should override `virtual void received()` method to process data.
/// Overridden method should get data pointer by call data(ovl), read the data
/// and immediatelly call `enqueue_buffer()` method.
class NIRVANA_NOVTABLE BufferPool :
	public CompletionPortReceiver
{
private:
	/// Deprecated
	BufferPool (const BufferPool&);
	/// Deprecated
	BufferPool& operator = (const BufferPool&);

public:
	BufferPool () NIRVANA_NOEXCEPT :
		handle_ (INVALID_HANDLE_VALUE),
		begin_ (nullptr),
		end_ (nullptr),
		buffer_size_ (0)
	{}

	/// Returns data buffer pointer for specified OVERLAPPED pointer.
	static void* data (OVERLAPPED* ovl) NIRVANA_NOEXCEPT
	{
		return ovl + 1;
	}

	/// Override in derived class.
	virtual void enqueue_buffer (OVERLAPPED* ovl) NIRVANA_NOEXCEPT = 0;

protected:
	void start (CompletionPort& port, size_t buffer_count, size_t buffer_size);
	virtual void terminate () NIRVANA_NOEXCEPT;

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

protected:
	HANDLE handle_;

private:
	OVERLAPPED* begin_;
	OVERLAPPED* end_;
	size_t buffer_size_;
};

}
}
}

#endif
