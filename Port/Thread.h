// Nirvana project
// Windows implementation.
// Port::Thread class. Thread object base.

#ifndef NIRVANA_CORE_PORT_THREAD_H_
#define NIRVANA_CORE_PORT_THREAD_H_

extern "C" __declspec (dllimport)
int __stdcall CloseHandle (void* hObject);

typedef unsigned long (__stdcall *PTHREAD_START_ROUTINE) (void* lpThreadParameter);

namespace Nirvana {
namespace Core {

class ExecContext;

namespace Port {

class Thread
{
	/// Deprecated
	Thread (const Thread&) = delete;
	/// Deprecated
	Thread& operator = (const Thread&) = delete;

public:
	static Thread* current ()
	{
		return current_;
	}

	//! \fn	void Thread::attach ()
	//!
	//! \brief	Attaches current thread to the Thread object.

	void attach ();
	void detach ();

	template <class T>
	void create (T* p, int priority = 0) // THREAD_PRIORITY_NORMAL = 0
	{
		create ((PTHREAD_START_ROUTINE)T::thread_proc, p, priority);
	}

	void* handle () const
	{
		return handle_;
	}

	void join () const;
	void thread_proc ();

protected:
	Thread () :
		handle_ (nullptr)
	{}

	~Thread ()
	{
		if (handle_)
			CloseHandle (handle_);
	}

private:
	void create (PTHREAD_START_ROUTINE thread_proc, void* param, int priority);

private:
	static thread_local Thread* current_;

	void* handle_;
};

}
}
}

#endif
