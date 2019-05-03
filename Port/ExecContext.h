// Nirvana project.
// Windows implementation.
// Execution context (fiber).

#ifndef NIRVANA_CORE_PORT_EXECCONTEXT_H_
#define NIRVANA_CORE_PORT_EXECCONTEXT_H_

#include <Nirvana/Nirvana.h>
#include <basetsd.h>

typedef void (__stdcall *PFIBER_START_ROUTINE)(void* lpFiberParameter);

extern "C" __declspec (dllimport)
void* __stdcall CreateFiber (
			SIZE_T dwStackSize,
			PFIBER_START_ROUTINE lpStartAddress,
			void* lpParameter
		);

extern "C" __declspec (dllimport)
void __stdcall DeleteFiber (void*);

extern "C" __declspec (dllimport)
void* __stdcall ConvertThreadToFiber (void* lpParameter);

extern "C" __declspec (dllimport)
int __stdcall ConvertFiberToThread ();

extern "C" __declspec (dllimport)
void __stdcall SwitchToFiber (void* lpFiber);

namespace Nirvana {
namespace Core {

class ExecDomain;

namespace Port {

class ExecContext
{
public:
	ExecContext () :
		fiber_ (CreateFiber (0, fiber_proc, nullptr))
	{
		if (!fiber_)
			throw CORBA::NO_MEMORY ();
	}

	ExecContext (void* fiber) :
		fiber_ (fiber)
	{}

	~ExecContext()
	{
		if (fiber_)
			DeleteFiber (fiber_);
	}

	void convert_to_fiber ()
	{
		assert (!fiber_);
		fiber_ = ConvertThreadToFiber (nullptr);
	}

	void convert_to_thread ()
	{
		assert (fiber_);
		fiber_ = nullptr;
		verify (ConvertFiberToThread ());
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

	void* detach ()
	{
		void* f = fiber_;
		fiber_ = nullptr;
		return f;
	}

	static void __stdcall fiber_proc (void*);

private:
	void* fiber_;
};

}
}
}

#endif
