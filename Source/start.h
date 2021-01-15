#ifndef NIRVANA_CORE_WINDOWS_START_H_
#define NIRVANA_CORE_WINDOWS_START_H_

#include "initialize.h"
#include "CmdLineParser.h"
#include <corecrt_startup.h>
#include <vcstartup_internal.h>

namespace Nirvana {
namespace Core {
namespace Windows {

template <int (*mainfn) (int, char**)> inline
int start ()
{
  // The /GS security cookie must be initialized before any exception handling
  // targeting the current image is registered.  No function using exception
  // handling can be called in the current image until after this call:
  __security_init_cookie ();

  if (!initialize ())
    return -1;

  if (!__scrt_initialize_crt (__scrt_module_type::exe))
    __scrt_fastfail (FAST_FAIL_FATAL_APP_EXIT);

  // Initialize C
  if (_initterm_e (__xi_a, __xi_z) != 0)
    return 255;

  // Initialize C++
  _initterm (__xc_a, __xc_z);

  // If this module has any dynamically initialized __declspec(thread)
  // variables, then we invoke their initialization for the primary thread
  // used to start the process:
  /*
  _tls_callback_type const* const tls_init_callback = __scrt_get_dyn_tls_init_callback ();
  if (*tls_init_callback != nullptr && __scrt_is_nonwritable_in_current_image (tls_init_callback)) {
    (*tls_init_callback)(nullptr, DLL_THREAD_ATTACH, nullptr);
  }
  */
  assert (!*__scrt_get_dyn_tls_init_callback ());

  // If this module has any thread-local destructors, register the
  // callback function with the Unified CRT to run on exit.
  /*
  _tls_callback_type const* const tls_dtor_callback = __scrt_get_dyn_tls_dtor_callback ();
  if (*tls_dtor_callback != nullptr && __scrt_is_nonwritable_in_current_image (tls_dtor_callback)) {
    _register_thread_local_exe_atexit_callback (*tls_dtor_callback);
  }
  */
  assert (!*__scrt_get_dyn_tls_dtor_callback ());

  CmdLineParser cmdline;

  int ret = mainfn (cmdline.argc (), cmdline.argv ());
  _cexit ();
  return ret;
}

}
}
}

#define NIRVANA_MAIN(mfunc) extern "C" int mfunc##_startup () { return Nirvana::Core::Windows::start <Nirvana::Core::Windows::mfunc> (); }

#endif
