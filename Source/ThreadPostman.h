// Nirvana project.
// Windows implementation.
#ifndef NIRVANA_CORE_THREADPOSTMAN_H_
#define NIRVANA_CORE_THREADPOSTMAN_H_

#include "Thread.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class CompletionPort;

/// ThreadPostman class. Thread for thread pool controlled by CompletionPort.
class ThreadPostman :
	public Port::Thread
{
public:
	ThreadPostman (CompletionPort& completion_port) :
		completion_port_ (completion_port)
	{}

protected:
	friend class Thread;
	static unsigned long __stdcall thread_proc (ThreadPostman* _this);

private:
	CompletionPort& completion_port_;
};

}
}
}

#endif
