#include <Windows.h>
#include <vcruntime_startup.h>
#include <corecrt_startup.h>
#include <vcruntime/internal_shared.h>

namespace Nirvana {
namespace Core {
namespace Windows {

int main ();

}
}
}

extern "C"
int __stdcall nirvana_main ()
{
	// The /GS security cookie must be initialized before any exception handling
	// targeting the current image is registered.  No function using exception
	// handling can be called in the current image until after this call:
	__security_init_cookie ();

	__isa_available_init ();

	__try {
/*
		if (_initterm_e (__xi_a, __xi_z) != 0)
			return 255;

		_initterm (__xc_a, __xc_z);
*/
		int const main_result = Nirvana::Core::Windows::main ();
		exit (main_result);

	} __except (_seh_filter_exe (GetExceptionCode (), GetExceptionInformation ()))
	{
		// Note:  We should never reach this except clause.
		int const main_result = GetExceptionCode ();

		_exit (main_result);
	}

}
