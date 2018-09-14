// Nirvana project
// Windows implementation.
// ThreadWindows class. Thread object base.

#ifndef NIRVANA_CORE_WINDOWS_THREADWINDOWS_H_
#define NIRVANA_CORE_WINDOWS_THREADWINDOWS_H_

#include <Nirvana.h>
#include "../ExecDomain.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class ThreadWindows
{
	/// Deprecated
	ThreadWindows (const ThreadWindows&);
	/// Deprecated
	ThreadWindows& operator = (const ThreadWindows&);

protected:
	static void initialize ()
	{
		tls_current_ = TlsAlloc ();
	}

	static void terminate ()
	{
		TlsFree (tls_current_);
	}

	static ThreadWindows* current ()
	{
		return (ThreadWindows*)TlsGetValue (tls_current_);
	}

	ThreadWindows () :
		handle_ (nullptr)
	{}

	~ThreadWindows ()
	{
		if (handle_)
			CloseHandle (handle_);
	}

	static void thread_proc (ThreadWindows* _this)
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
		WaitForSingleObject (handle_, INFINITE);
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
