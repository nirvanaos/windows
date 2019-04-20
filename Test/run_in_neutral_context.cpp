// Mock implementation for Nirvana::Core::run_in_neutral_context()
#include <Nirvana/Runnable_c.h>
#include <CORBA/CORBA.h>
#include "../Source/win32.h"

namespace Test {

struct FiberParam
{
	Nirvana::Runnable_ptr runnable;
	void* source_fiber;
	CORBA::Nirvana::Environment environment;
};

void CALLBACK neutral_fiber_proc (void* param)
{
	FiberParam* fp = (FiberParam*)param;
	(fp->runnable->_epv().epv.run) (fp->runnable, &fp->environment);
	SwitchToFiber (fp->source_fiber);
}

}

namespace Nirvana {
namespace Core {

void run_in_neutral_context (Runnable_ptr runnable)
{
	Test::FiberParam fp;
	fp.source_fiber = GetCurrentFiber ();
	fp.runnable = runnable;
	void* fiber = CreateFiber (0, Test::neutral_fiber_proc, &fp);
	if (!fiber)
		throw CORBA::INTERNAL ();
	SwitchToFiber (fiber);
	DeleteFiber (fiber);
	fp.environment.check ();
}

}
}
