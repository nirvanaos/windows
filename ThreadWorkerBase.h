// Nirvana project
// Windows implementation.
// ThreadWorkerBase class.
// Platform-specific worker thread implementation.

#ifndef NIRVANA_CORE_WINDOWS_THREADWORKERBASE_H_
#define NIRVANA_CORE_WINDOWS_THREADWORKERBASE_H_

#include "../Thread.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class ThreadWorkerBase :
	public Thread
{
public:
	ThreadWorkerBase ()
	{
		create (thread_proc, this);
	}

	~ThreadWorkerBase ()
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
