// Nirvana project
// Windows implementation.
// ThreadBase class.

#ifndef NIRVANA_CORE_WINDOWS_THREADBASE_H_
#define NIRVANA_CORE_WINDOWS_THREADBASE_H_

#include <Nirvana.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class ThreadBase
{
protected:
	static void initialize ()
	{
		tls_current_ = TlsAlloc ();
	}

	static void terminate ()
	{
		TlsFree (tls_current_);
	}

	static ThreadBase* current ()
	{
		return (ThreadBase*)TlsGetValue (tls_current_);
	}

	void switch_to_neutral () const
	{
		SwitchToFiber (neutral_fiber_);
	}

	void thread_init ()
	{
		TlsSetValue (tls_current_, this);
		neutral_fiber_ = ConvertThreadToFiber (nullptr);
	}

	void thread_term ()
	{
		ConvertFiberToThread ();
	}

	void create (LPTHREAD_START_ROUTINE thread_proc, void* param);

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
	HANDLE handle_;
	void* neutral_fiber_;
	static DWORD tls_current_;
};

}
}
}

#endif
