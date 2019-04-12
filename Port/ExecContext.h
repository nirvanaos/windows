// Nirvana project.
// Windows implementation.
// Execution context (fiber).

#ifndef NIRVANA_CORE_PORT_EXECCONTEXT_H_
#define NIRVANA_CORE_PORT_EXECCONTEXT_H_

#include "../Source/Win32.h"
#include <Nirvana/Nirvana.h>

namespace Nirvana {
namespace Core {

class ExecDomain;

namespace Port {

class ExecContext
{
public:
	enum CreationType
	{
		CREATE_DEFAULT,
		CREATE_NONE,
		CREATE_CONVERT
	};

	ExecContext (CreationType type = CREATE_DEFAULT);

	~ExecContext()
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

	static ExecContext* current ()
	{
		return (ExecContext*)GetFiberData ();
	}

	void switch_to ()
	{
		SwitchToFiber (fiber_);
	}

	void attach (void* fiber)
	{
		assert (!fiber_);
		fiber_ = fiber;
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
