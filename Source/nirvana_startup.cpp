#include <Heap.h>

#define _VCRT_ALLOW_INTERNALS
#define _SCRT_STARTUP_MAIN
#define main nirvana_main
#include <exe_common.inl>

#include "Console.h"
#include <exception>

using Nirvana::Core::Heap;
using Nirvana::Core::Windows::Console;

extern "C" int nirvana_startup ()
{
  // The /GS security cookie must be initialized before any exception handling
  // targeting the current image is registered.  No function using exception
  // handling can be called in the current image until after this call:
  __security_init_cookie ();

  try {
    Heap::initialize ();
  } catch (const std::exception& ex) {
    Console::write (ex.what ());
    Console::write ("\n");
    return -1;
  }

  return __scrt_common_main_seh ();
}
