#ifndef NIRVANA_CORE_WINDOWS_EXECCONTEXTINTERNAL_H_
#define NIRVANA_CORE_WINDOWS_EXECCONTEXTINTERNAL_H_

#include "../Port/ExecContext.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

inline void ExecContext::convert_to_fiber ()
{
	assert (!fiber_);
	// If dwFlags parameter is zero, the floating - point state on x86 systems is not switched and data 
	// can be corrupted if a fiber uses floating - point arithmetic.
	// This causes faster switching. Neutral execution context does not use floating - point arithmetic.
	if (!(fiber_ = ConvertThreadToFiberEx (nullptr, 0)))
		throw_NO_MEMORY ();
}

inline void ExecContext::convert_to_thread () NIRVANA_NOEXCEPT
{
	assert (fiber_);
	fiber_ = nullptr;
	verify (ConvertFiberToThread ());
}

}
}
}

#endif
