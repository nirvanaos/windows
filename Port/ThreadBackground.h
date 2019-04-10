// Nirvana project
// Windows implementation.
// ThreadBackgroundBase class.
// Platform-specific background thread implementation.

#ifndef NIRVANA_CORE_WINDOWS_THREADBACKGROUNDBASE_H_
#define NIRVANA_CORE_WINDOWS_THREADBACKGROUNDBASE_H_

#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Port {

class ThreadBackground :
	public Core::Thread
{
public:
	ThreadBackground ()
	{
		Port::Thread::create (this, thread_proc);
	}

	~ThreadBackground ()
	{
		close ();
	}

private:
	static DWORD WINAPI thread_proc (void* _this);
};

}
}
}

#endif
