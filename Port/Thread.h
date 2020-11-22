// Nirvana project
// Windows implementation.
// Port::Thread class. Thread object base.

#ifndef NIRVANA_CORE_PORT_THREAD_H_
#define NIRVANA_CORE_PORT_THREAD_H_

#include "ExecContext.h"

typedef unsigned long (__stdcall *PTHREAD_START_ROUTINE) (void* lpThreadParameter);

extern "C" __declspec (dllimport)
void* __stdcall TlsGetValue (unsigned long dwTlsIndex);

namespace Nirvana {
namespace Core {

class Thread;

namespace Port {

class Thread
{
	Thread (const Thread&) = delete;
	Thread& operator = (const Thread&) = delete;

	///@{
	/// Members called from Core.
public:
	/// \returns The current Core::Thread object.
	static Core::Thread* current () NIRVANA_NOEXCEPT
	{
		return (Core::Thread*)TlsGetValue (current_);
	}

	static Core::ExecContext* context () NIRVANA_NOEXCEPT
	{
		return ExecContext::current ();
	}
	///@}

public:
	static void initialize ();
	static void terminate ();

	template <class T>
	void create (T* p, int priority) // THREAD_PRIORITY_NORMAL = 0
	{
		create ((PTHREAD_START_ROUTINE)T::thread_proc, p, priority);
	}

	void join () const;

protected:
	Thread () NIRVANA_NOEXCEPT :
		handle_ (nullptr)
	{}

	~Thread ();

	static void current (Core::Thread& core_thread);

private:
	void create (PTHREAD_START_ROUTINE thread_proc, void* param, int priority);

protected:
	static unsigned long current_;

	void* handle_;
};

}
}
}

#endif
