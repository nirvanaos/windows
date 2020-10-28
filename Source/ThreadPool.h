// Nirvana project.
// Windows implementation.
// ThreadPool template class.

#ifndef NIRVANA_CORE_WINDOWS_THREADPOOL_H_
#define NIRVANA_CORE_WINDOWS_THREADPOOL_H_

#include <Heap.h>
#include "CompletionPortReceiver.h"

namespace Nirvana {
namespace Core {
namespace Windows {

/// Windows I/O completion port wrapper.
class CompletionPort
{
private:
	/// Deprecated
	CompletionPort (const CompletionPort&);
	/// Deprecated
	CompletionPort& operator = (const CompletionPort&);

public:
	CompletionPort ();

	~CompletionPort ()
	{
		if (completion_port_)
			CloseHandle (completion_port_);
	}

	/// Add a CompletionPortReceiver object listening this completion port.
	void add_receiver (HANDLE hf, CompletionPortReceiver& receiver)
	{
		create (hf, &receiver);
	}

	/// Post an I/O completion packet to an I/O completion port.
	void post (CompletionPortReceiver& receiver, OVERLAPPED* param, DWORD size)
	{
		verify (PostQueuedCompletionStatus (completion_port_, size, (ULONG_PTR)&receiver, param));
	}

	/// On close completion port all threads will return with `ERROR_ABANDONED_WAIT_0` error code.
	virtual void terminate ()
	{
		HANDLE port = completion_port_;
		completion_port_ = nullptr;
		if (port)
			CloseHandle (port);
	}

	unsigned thread_count () const
	{
		return thread_count_;
	}

	/// Worker thread procedure.
	void thread_proc ();

protected:
	/// Ensure that port exists.
	void start ()
	{
		if (!completion_port_)
			create (INVALID_HANDLE_VALUE, nullptr);
	}

private:
	bool dispatch ();
	void create (HANDLE hfile, CompletionPortReceiver* receiver);

private:
	HANDLE completion_port_;
	const unsigned thread_count_;
};

//! Template class for thread pool controlled by a completion port.
//! \tparam Thread Thread class.
//! Thread constructor gets reference to CompletionPort as parameter.
//! Thread class must have method Thread::create(int priority);
//! Thread procedure must call CompletionPort::thread_proc() method.

template <class Thread>
class ThreadPool :
	public CompletionPort
{
	typedef CoreAllocator <Thread> Allocator;

public:
	ThreadPool () :
		CompletionPort (),
		threads_ (nullptr)
	{
		Allocator allocator;
		threads_ = allocator.allocate (CompletionPort::thread_count ());
		Thread* p = threads_;
		try {
			for (Thread* end = p + CompletionPort::thread_count (); p != end; ++p) {
				allocator.construct (p, *static_cast <CompletionPort*> (this));
			}
		} catch (...) {
			while (p != threads_) {
				allocator.destroy (--p);
			}
			allocator.deallocate (threads_, CompletionPort::thread_count ());
			throw;
		}
	}

	~ThreadPool ()
	{
		Allocator allocator;
		for (Thread* p = threads_, *end = p + CompletionPort::thread_count (); p != end; ++p) {
			allocator.destroy (p);
		}
		allocator.deallocate (threads_, CompletionPort::thread_count ());
	}

	Thread* threads ()
	{
		return threads_;
	}

	unsigned thread_count () const
	{
		return CompletionPort::thread_count ();
	}

	//! Create and start threads.
	void start (int priority)
	{
		CompletionPort::start ();
		for (Thread* p = threads_, *end = p + thread_count (); p != end; ++p) {
			p->create (p, priority);
		}
	}

	//! Terminate threads.
	virtual void terminate ()
	{
		CompletionPort::terminate ();
		for (Thread* p = threads_, *end = p + thread_count (); p != end; ++p) {
			p->join ();
		}
	}

private:
	Thread* threads_;
};

}
}
}

#endif
