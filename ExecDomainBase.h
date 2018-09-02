// Nirvana project.
// Windows implementation.
// Execution domain (fiber).

#ifndef NIRVANA_CORE_WINDOWS_EXECDOMAINBASE_H_
#define NIRVANA_CORE_WINDOWS_EXECDOMAINBASE_H_

#include "Win32.h"

namespace Nirvana {
namespace Core {

class ExecDomain;

namespace Windows {

class ExecDomainBase
{
protected:
	ExecDomainBase ()
	{
		fiber_ = CreateFiber (0, fiber_proc, this);
	}

	ExecDomainBase (void* fiber) :
		fiber_ (fiber)
	{}

	~ExecDomainBase()
	{
		DeleteFiber (fiber_);
	}

	static ExecDomainBase* current ()
	{
		return (ExecDomainBase*)GetFiberData ();
	}

	void switch_to ()
	{
		SwitchToFiber (fiber_);
	}

private:
	static void CALLBACK fiber_proc (void* param);

private:
	void* fiber_;
};

}
}
}

#endif
