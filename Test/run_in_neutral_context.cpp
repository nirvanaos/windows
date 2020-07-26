// Mock implementation for Nirvana::Core::run_in_neutral_context()
#include <core.h>
#include <Runnable.h>
#include <CORBA/CORBA.h>
#include "../Source/win32.h"

namespace Test {

struct FiberParam
{
	Nirvana::Core::Core_var <Nirvana::Core::Runnable> runnable;
	void* source_fiber;
	CORBA::Nirvana::Environment environment;
};

void CALLBACK neutral_fiber_proc (void* param)
{
	FiberParam* fp = (FiberParam*)param;
	try {
		fp->runnable->run ();
	} catch (const CORBA::Exception& ex) {
		CORBA::Nirvana::set_exception (&fp->environment, ex);
	} catch (...) {
		CORBA::Nirvana::set_unknown_exception (&fp->environment);
	}
	SwitchToFiber (fp->source_fiber);
}

}

namespace Nirvana {
namespace Core {

void run_in_neutral_context (Runnable& runnable)
{
	Test::FiberParam fp;
	fp.source_fiber = GetCurrentFiber ();
	runnable._add_ref ();
	fp.runnable = &runnable;
	void* fiber = CreateFiber (Windows::NEUTRAL_FIBER_STACK_SIZE, Test::neutral_fiber_proc, &fp);
	if (!fiber)
		throw CORBA::INTERNAL ();
	SwitchToFiber (fiber);
	DeleteFiber (fiber);
	fp.environment.check ();
}

}
}
