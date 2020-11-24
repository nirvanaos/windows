#ifndef NIRVANA_CORE_WINDOWS_EXECCONTEXT_INL_
#define NIRVANA_CORE_WINDOWS_EXECCONTEXT_INL_

#include "../Port/ExecContext.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

inline
void ExecContext::initialize ()
{
	current_ = FlsAlloc (nullptr);
}

inline
void ExecContext::terminate ()
{
	FlsFree (current_);
}

inline
void ExecContext::current (Core::ExecContext* context)
{
	FlsSetValue (current_, context);
}

inline
void ExecContext::convert_to_thread () NIRVANA_NOEXCEPT
{
	assert (fiber_);
	fiber_ = nullptr;
	verify (ConvertFiberToThread ());
}

}
}
}

#endif
