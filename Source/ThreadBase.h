// Nirvana project
// Windows implementation.
// Port::Thread class. Thread object base.

#ifndef NIRVANA_CORE_PORT_THREADBASE_H_
#define NIRVANA_CORE_PORT_THREADBASE_H_

typedef unsigned long (__stdcall *PTHREAD_START_ROUTINE) (void* lpThreadParameter);

namespace Nirvana {
namespace Core {

class Thread;

namespace Windows {

class ThreadBase
{
	/// Deprecated
	ThreadBase (const ThreadBase&) = delete;
	/// Deprecated
	ThreadBase& operator = (const ThreadBase&) = delete;

public:
	static Thread* current ()
	{
		return current_;
	}

	template <class T>
	void create (T* p, int priority = 0) // THREAD_PRIORITY_NORMAL = 0
	{
		create ((PTHREAD_START_ROUTINE)T::thread_proc, p, priority);
	}

	void join () const;

protected:
	ThreadBase () :
		handle_ (nullptr)
	{}

	~ThreadBase ();

private:
	void create (PTHREAD_START_ROUTINE thread_proc, void* param, int priority);

protected:
	static thread_local Core::Thread* current_;

	void* handle_;
};

}
}
}

#endif
