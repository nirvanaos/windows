#include <Nirvana/Nirvana.h>
#include <Nirvana/Module.h>
#include <corecrt_startup.h>
#include <internal_shared.h>

extern "C" void __cdecl _cexit ()
{
  _initterm (__xp_a, __xp_z);
  _initterm (__xt_a, __xt_z);
}

extern "C" int __cdecl _initialize_onexit_table (_onexit_table_t* const table)
{
  return 0;
}

extern "C" int __cdecl _execute_onexit_table (_onexit_table_t* const table)
{
  return 0;
}

extern "C" int __cdecl _register_onexit_function (_onexit_table_t* const table, _onexit_t const function)
{
  return 0;
}

extern "C" int __cdecl _crt_atexit (_PVFV const function)
{
  try {
    Nirvana::module->atexit (function);
  } catch (...) {
    return ENOMEM;
  }
  return 0;
}

extern "C" int __cdecl _crt_at_quick_exit (_PVFV const function)
{
  return ENOSYS;
}

