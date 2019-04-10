// Nirvana project
// Windows implementation.
// ThreadBackgroundBase class.
// Platform-specific background thread implementation.

#ifndef NIRVANA_CORE_WINDOWS_THREADBACKGROUNDBASE_H_
#define NIRVANA_CORE_WINDOWS_THREADBACKGROUNDBASE_H_

#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class ThreadBackgroundBase :
	public Thread
{
public:
	ThreadBackgroundBase ()
	{
		create (thread_proc, this);
	}

	~ThreadBackgroundBase ()
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
