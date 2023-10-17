#include <corecrt_startup.h>
#include <Windows.h>

extern "C" int __cdecl _seh_filter_dll (
  unsigned long       const xcptnum,
  PEXCEPTION_POINTERS const pxcptinfoptrs
)
{
  return EXCEPTION_CONTINUE_SEARCH;
}

extern "C" errno_t __cdecl _configure_narrow_argv (_crt_argv_mode const mode)
{
  return 0;
}

