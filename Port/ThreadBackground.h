// Nirvana project
// Windows implementation.
// ThreadBackgroundBase class.
// Platform-specific background thread implementation.

#ifndef NIRVANA_CORE_PORT_THREADBACKGROUND_H_
#define NIRVANA_CORE_PORT_THREADBACKGROUND_H_

#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Port {

class ThreadBackground :
	public Core::Thread
{
	///@{
	/// Members called from Core.
protected:
	ThreadBackground ();
	~ThreadBackground ();

	/// Create thread
	void create ();

	/// Suspend execution and wait for resume ().
	void suspend ();

	/// Continue execution.
	void resume ();

	/// Temparary boost thread priority above the worker threads priority.
	void priority_boost ();

	/// Restore priority after the priority_boost () call.
	void priority_restore ();
	///@}

private:
	friend class Nirvana::Core::Port::Thread;
	static unsigned long __stdcall thread_proc (ThreadBackground* _this);

private:
	void* event_;
};

}
}
}

#endif
