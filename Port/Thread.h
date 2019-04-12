// Nirvana project
// Windows implementation.
// Port::Thread class. Thread object base.

#ifndef NIRVANA_CORE_PORT_THREAD_H_
#define NIRVANA_CORE_PORT_THREAD_H_

#include <Nirvana/Nirvana.h>
#include "../Source/win32.h"

namespace Nirvana {
namespace Core {

class ExecContext;

namespace Port {

class Thread
{
	/// Deprecated
	Thread (const Thread&);
	/// Deprecated
	Thread& operator = (const Thread&);

protected:
	static void initialize ()
	{
		tls_current_ = TlsAlloc ();
	}

	static void terminate ()
	{
		TlsFree (tls_current_);
	}

	static Thread* current ()
	{
		assert (tls_current_);
		return (Thread*)TlsGetValue (tls_current_);
	}

	Thread () :
		handle_ (nullptr)
	{}

	~Thread ()
	{
		if (handle_)
			CloseHandle (handle_);
	}

	//! \fn	void Thread::attach ()
	//!
	//! \brief	Attaches current thread to the Thread object.

	void attach ()
	{
		assert (!handle_);
		BOOL ok = DuplicateHandle (GetCurrentProcess (), GetCurrentThread (), GetCurrentProcess (), &handle_, 0, FALSE, DUPLICATE_SAME_ACCESS);
		assert (ok);
		TlsSetValue (tls_current_, this);
	}

	void detach ()
	{
		if (handle_) {
			assert (GetThreadId (handle_) == GetCurrentThreadId ());
			TlsSetValue (tls_current_, nullptr);
			CloseHandle (handle_);
			handle_ = nullptr;
		}
	}

	static void thread_proc (Thread* _this)
	{
		TlsSetValue (tls_current_, _this);
	}

	template <class T>
	void create (T* p, SIZE_T stack_size = 0, int priority = THREAD_PRIORITY_NORMAL)
	{
		create (stack_size, (LPTHREAD_START_ROUTINE)T::thread_proc, p, priority);
	}

	HANDLE handle () const
	{
		return handle_;
	}

	void close ()
	{
		CloseHandle (handle_);
	}

	void join () const
	{
		if (handle_)
			WaitForSingleObject (handle_, INFINITE);
	}

	void boost_priority (bool boost)
	{
		SetThreadPriority (handle_, boost ? Windows::BOOSTED_THREAD_PRIORITY : THREAD_PRIORITY_NORMAL);
	}

	/// Returns special "neutral" execution context with own stack and CPU state.
	virtual Core::ExecContext* neutral_context ()
	{
		assert (false);
		return nullptr;
	}

private:
	void create (SIZE_T stack_size, LPTHREAD_START_ROUTINE thread_proc, void* param, int priority);

private:
	static DWORD tls_current_;

	HANDLE handle_;
};

}
}
}

#endif
