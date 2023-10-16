#include <Windows.h>

extern "C" int __cdecl _seh_filter_dll (
  unsigned long       const xcptnum,
  PEXCEPTION_POINTERS const pxcptinfoptrs
)
{
  return EXCEPTION_CONTINUE_SEARCH;
}
