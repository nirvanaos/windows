#include <stdio.h>

extern "C" FILE * __cdecl __acrt_iob_func (unsigned const id)
{
  return (FILE*)(uintptr_t)(id + 1);
}
