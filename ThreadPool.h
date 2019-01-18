// Nirvana project.
// Windows implementation.
// ThreadPool template class.

#ifndef NIRVANA_CORE_WINDOWS_THREADPOOL_H_
#define NIRVANA_CORE_WINDOWS_THREADPOOL_H_

#include <Nirvana/Nirvana.h>
#include <ORB.h>
#include "CompletionPortReceiver.h"
#include "../core.h"

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

/// Template class for thread pool controlled by a completion port.
/// \tparam Thread Thread class. Thread must start in the constructor and join in the destructor.
/// Thread constructor gets reference to CompletionPort as first parameter and priority as second.
/// Thread procedure must call CompletionPort::thread_proc() method.
template <class Thread>
class ThreadPool :
	public CompletionPort
{
	typedef CoreAllocator <Thread> Allocator;

public:
	ThreadPool () :
		CompletionPort (),
		threads_ (nullptr),
		thread_count_ (0)
	{}

	/// Create and start threads.
	void start (int priority);

	/// Terminate threads.
	virtual void terminate ();

private:
	Thread* threads_;
	unsigned thread_count_;
};

template <class Thread>
void ThreadPool <Thread>::start (int priority)
{
	if (!threads_) {
		try {
			CompletionPort::start ();
			Allocator allocator;
			threads_ = allocator.allocate (CompletionPort::thread_count ());
			for (Thread* p = threads_, *end = p + CompletionPort::thread_count (); p != end; ++p) {
				allocator.construct (p, *static_cast <CompletionPort*> (this), priority);
				++thread_count_;
			}
		} catch (...) {
			terminate ();
			throw;
		}
	}
}

template <class Thread>
void ThreadPool <Thread>::terminate ()
{
	CompletionPort::terminate ();
	// On close completion port all threads will return with ERROR_ABANDONED_WAIT_0 error code.
	if (threads_) {
		Allocator allocator;
		for (Thread* p = threads_, *end = p + thread_count_; p != end; ++p) {
			allocator.destroy (p); // Thread joins in destructor.
		}
		allocator.deallocate (threads_, CompletionPort::thread_count ());
		threads_ = nullptr;
	}
}

}
}
}

#endif
