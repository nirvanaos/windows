// Nirvana project.
// Windows implementation.
// Execution context (fiber).

#ifndef NIRVANA_CORE_WINDOWS_EXECCONTEXTWINDOWS_H_
#define NIRVANA_CORE_WINDOWS_EXECCONTEXTWINDOWS_H_

#include "Win32.h"
#include <ORB.h>

namespace Nirvana {
namespace Core {

class ExecDomain;

namespace Windows {

class ExecContextWindows
{
public:
	enum CreationType
	{
		CREATE_DEFAULT,
		CREATE_NEUTRAL,
		CREATE_NONE
	};

	ExecContextWindows (CreationType type = CREATE_DEFAULT);

	~ExecContextWindows()
	{
		if (fiber_)
			DeleteFiber (fiber_);
	}

	void convert_to_fiber ()
	{
		assert (!fiber_);
		fiber_ = ConvertThreadToFiber (this);
	}

	void convert_to_thread ()
	{
		assert (fiber_);
		fiber_ = nullptr;
		verify (ConvertFiberToThread ());
	}

	static ExecContextWindows* current ()
	{
		return (ExecContextWindows*)GetFiberData ();
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
