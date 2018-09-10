// Nirvana project.
// Windows implementation.
// ThreadPool template class.

#ifndef NIRVANA_CORE_WINDOWS_THREADPOOL_H_
#define NIRVANA_CORE_WINDOWS_THREADPOOL_H_

#include <Nirvana.h>
#include <ORB.h>
#include "CompletionPortReceiver.h"
#include "SystemInfo.h"
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
	CompletionPort () :
		completion_port_ (nullptr),
		thread_count_ (SystemInfo::hardware_concurrency ())
	{}

	~CompletionPort ()
	{
		if (completion_port_)
			CloseHandle (completion_port_);
	}

	void add_receiver (HANDLE hf, CompletionPortReceiver& receiver)
	{
		create (hf, &receiver);
	}
	
	void create ()
	{
		if (!completion_port_)
			create (INVALID_HANDLE_VALUE, nullptr);
	}

	void post (CompletionPortReceiver& receiver, OVERLAPPED* param, DWORD size)
	{
		verify (PostQueuedCompletionStatus (completion_port_, size, (ULONG_PTR)&receiver, param));
	}

	/// On close completion port all threads will return with `ERROR_ABANDONED_WAIT_0` error code.
	void terminate ()
	{
		HANDLE port = completion_port_;
		completion_port_ = nullptr;
		CloseHandle (port);
	}

	unsigned thread_count () const
	{
		return thread_count_;
	}

	/// Worker threads have to call dispatch() while it returns `true`.
	bool dispatch ()
	{
		if (completion_port_) {
			ULONG_PTR key;
			OVERLAPPED* ovl;
			DWORD size;
			if (GetQueuedCompletionStatus (completion_port_, &size, &key, &ovl, INFINITE)) {
				reinterpret_cast <CompletionPortReceiver*> (key)->received (ovl, size);
				return true;
			} else if (completion_port_) {
				throw ::CORBA::INTERNAL ();
			}
		}
		return false;
	}

private:
	void create (HANDLE hfile, CompletionPortReceiver* receiver)
	{
		HANDLE port = CreateIoCompletionPort (hfile, completion_port_, (ULONG_PTR)receiver, thread_count ());
		if (!port)
			throw ::CORBA::INITIALIZE ();
		completion_port_ = port;
	}

private:
	HANDLE completion_port_;
	const unsigned thread_count_;
};

/// Template class for thread pool controlled by a completion port.
/// \tparam Thread Thread class. Thread must start in the constructor and join in the destructor.
/// Thread constructor gets reference to CompletionPort as parameter.
/// Thread procedure must call CompletionPort::dispatch() method while it returns `true`.
template <class Thread>
class ThreadPool :
	public CompletionPort
{
	typedef CoreAllocator <Thread> Allocator;

public:
	ThreadPool () :
		threads_ (nullptr),
		thread_count_ (0)
	{}

	/// Create and start threads.
	void start ()
	{
		CompletionPort::create ();
		try {
			Allocator allocator;
			threads_ = allocator.allocate (CompletionPort::thread_count ());
			for (Thread* p = threads_, *end = p + CompletionPort::thread_count (); p != end; ++p) {
				allocator.construct (p, *static_cast <CompletionPort*> (this));
				++thread_count_;
			}
		} catch (...) {
			terminate ();
			throw;
		}
	}

	/// Terminate threads.
	void terminate ();

private:
	Thread* threads_;
	unsigned thread_count_;
};

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
