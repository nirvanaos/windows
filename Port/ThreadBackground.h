// Nirvana project
// Windows implementation.
// ThreadBackgroundBase class.
// Platform-specific background thread implementation.

#ifndef NIRVANA_CORE_PORT_THREADBACKGROUND_H_
#define NIRVANA_CORE_PORT_THREADBACKGROUND_H_

#include "../Source/Thread.h"

namespace Nirvana {
namespace Core {
namespace Port {

class ThreadBackground :
	public Thread
{
protected:
	ThreadBackground ();
	~ThreadBackground ();

	void create ()
	{
		Thread::create (this, 0); // THREAD_PRIORITY_NORMAL = 0
	}

	void suspend ();
	void resume ();

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
