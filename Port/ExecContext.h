// Nirvana project.
// Windows implementation.
// Execution context (fiber).

#ifndef NIRVANA_CORE_PORT_EXECCONTEXT_H_
#define NIRVANA_CORE_PORT_EXECCONTEXT_H_

#include <Nirvana/Nirvana.h>
#include <basetsd.h>

extern "C" __declspec (dllimport)
void __stdcall DeleteFiber (void*);

extern "C" __declspec (dllimport)
void __stdcall SwitchToFiber (void* lpFiber);

extern "C" __declspec (dllimport)
void* __stdcall FlsGetValue (unsigned long dwFlsIndex);


namespace Nirvana {
namespace Core {

class ExecContext;

namespace Port {

/// Implements execution context
class ExecContext
{
	///@{
	/// Members called from Core.
public:
	static Core::ExecContext* current ()
	{
		return (Core::ExecContext*)FlsGetValue (current_);
	}

	/// Constructor.
	/// 
	/// \param neutral `true` if neutral context is created.
	ExecContext (bool neutral = false);

	/// Destructor.
	~ExecContext ()
	{
		if (fiber_)
			DeleteFiber (fiber_);
	}

protected:
	/// Switch to this context.
	void switch_to () NIRVANA_NOEXCEPT
	{
		assert (fiber_);
		SwitchToFiber (fiber_);
	}

	///@}

public:
	ExecContext (void* fiber) :
		fiber_ (fiber)
	{}

	void convert_to_fiber ();
	void convert_to_thread () NIRVANA_NOEXCEPT;

	void attach (void* fiber) NIRVANA_NOEXCEPT
	{
		assert (!fiber_);
		fiber_ = fiber;
	}

	void* detach () NIRVANA_NOEXCEPT
	{
		void* f = fiber_;
		fiber_ = nullptr;
		return f;
	}

	static void initialize ();
	static void terminate ();

	static void current (Core::ExecContext& context);

	static void __stdcall fiber_proc (Core::ExecContext* context);

private:
	static unsigned long current_;

	void* fiber_;
};

}
}
}

#endif
