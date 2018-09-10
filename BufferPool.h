// Nirvana project.
// Windows implementation.
// BufferPool class.

#ifndef NIRVANA_CORE_WINDOWS_BUFFERPOOL_H_
#define NIRVANA_CORE_WINDOWS_BUFFERPOOL_H_

#include "ThreadPool.h"

namespace Nirvana {
namespace Core {
namespace Windows {

/// Derived class should override void received() method to process data.
/// Overridden method should get data pointer by call data(ovl), read the data
/// and immediatelly call enqueue_buffer() method.
class BufferPool :
	public CompletionPortReceiver
{
private:
	/// Deprecated
	BufferPool (const BufferPool&);
	/// Deprecated
	BufferPool& operator = (const BufferPool&);

public:
	BufferPool () :
		handle_ (INVALID_HANDLE_VALUE),
		begin_ (nullptr),
		end_ (nullptr),
		buffer_size_ (0)
	{}

	/// Returns data buffer pointer for specified OVERLAPPED pointer.
	static void* data (OVERLAPPED* ovl)
	{
		return ovl + 1;
	}

	/// Override in derived class.
	virtual void enqueue_buffer (OVERLAPPED* ovl) = 0;

protected:
	void start (CompletionPort& port, DWORD buffer_size);
	virtual void terminate ();

	size_t buffer_size () const
	{
		return buffer_size_;
	}

	OVERLAPPED* begin () const
	{
		return begin_;
	}

	OVERLAPPED* end () const
	{
		return end_;
	}

	OVERLAPPED* next (OVERLAPPED* ovl) const
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
