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
	/// \returns If thread class is derived from Core::Thread
	/// returns a pointer of the current Core::Thread object.
	/// Otherwise returns nullptr.
	static Core::Thread* current () NIRVANA_NOEXCEPT
	{
		return (Core::Thread*)TlsGetValue (current_);
	}

	/// Returns a pointer of the current execution context.
	/// For Windows it is implemented via FlsGetValue().
	/// If the system does not support fiber local storage,
	/// then pointer to current context must be stored as
	/// variable in Core::Thread class and updated for each
	/// ExecContext::switch_to() call. So context() can be
	/// implemented as Thread::current ().port ().context_.
	static Core::ExecContext* context () NIRVANA_NOEXCEPT
	{
		return ExecContext::current ();
	}
	///@}

public:
	static void initialize ();
	static void terminate ();

	static void current (Core::Thread* core_thread);

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
