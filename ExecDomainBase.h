// Nirvana project.
// Windows implementation.
// Execution domain (fiber).

#ifndef NIRVANA_CORE_WINDOWS_EXECDOMAINBASE_H_
#define NIRVANA_CORE_WINDOWS_EXECDOMAINBASE_H_

#include "Win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class ExecDomainBase
{
public:
	ExecDomainBase () :
		thread_handle_ (0) {
		fiber_ = CreateFiber (0, fiber_proc, this);
	}

	~ExecDomainBase()
	{
		DeleteFiber (fiber_);
	}

	static ExecDomainBase* current ()
	{
		return (ExecDomainBase*)GetFiberData ();
	}

private:
	static void CALLBACK fiber_proc (void* param);

private:
	void* fiber_;
	HANDLE thread_handle_;
};

}
}
}

#endif
