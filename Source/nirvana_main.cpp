#include <Heap.h>

namespace Nirvana {
namespace Core {
namespace Windows {

int main ();

}
}
}

enum class __scrt_module_type
{
	dll,
	exe
};

extern "C" bool __cdecl __scrt_initialize_crt(__scrt_module_type module_type);
extern "C" bool __cdecl __scrt_uninitialize_crt(bool is_terminating, bool from_exit);
extern "C" __declspec(noreturn) void __cdecl __scrt_fastfail (unsigned code);
extern "C" int __cdecl _seh_filter_exe (
	unsigned long       const xcptnum,
	PEXCEPTION_POINTERS const pxcptinfoptrs
);

extern "C"
int __stdcall nirvana_main ()
{
	// The /GS security cookie must be initialized before any exception handling
	// targeting the current image is registered.  No function using exception
	// handling can be called in the current image until after this call:
	__security_init_cookie ();

	if (!__scrt_initialize_crt (__scrt_module_type::exe))
		__scrt_fastfail (FAST_FAIL_FATAL_APP_EXIT);

	__try {
		int main_result = Nirvana::Core::Windows::main ();

		// Finally, we terminate the CRT:
		__scrt_uninitialize_crt (true, false);

		return main_result;

	} __except (_seh_filter_exe (GetExceptionCode (), GetExceptionInformation ()))
	{
		// Note:  We should never reach this except clause.
		int const main_result = GetExceptionCode ();

		return main_result;
	}

}
