#define CAT0(x, y) x##y
#define CAT(x, y) CAT0(x, y)
#define entry_point CAT(MAIN, _startup)
#define main CAT(MAIN, _main)

#define _VCRT_ALLOW_INTERNALS
#define _SCRT_STARTUP_MAIN
#include <exe_common.inl>

#include "initialize.h"

extern "C" int entry_point ()
{
  // The /GS security cookie must be initialized before any exception handling
  // targeting the current image is registered.  No function using exception
  // handling can be called in the current image until after this call:
  __security_init_cookie ();

  if (!Nirvana::Core::Windows::initialize ())
    return -1;

  return __scrt_common_main_seh ();
}

extern "C" int __cdecl main (int argc, char* argv [], char** envp)
{
  return Nirvana::Core::Windows::MAIN (argc, argv);
}
