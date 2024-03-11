#define _CRT_SECURE_NO_WARNINGS
#include <Nirvana/Nirvana.h>
#include <Nirvana/System.h>
#include <Nirvana/string_conv.h>

//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// _invalid_parameter
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C" void __cdecl _invalid_parameter (
  wchar_t const* const expression,
  wchar_t const* const function_name,
  wchar_t const* const file_name,
  unsigned int   const line_number,
  uintptr_t      const reserved
)
{
  std::string msg ("Invalid parameter");
  if (expression) {
    msg += ' ';
    Nirvana::append_utf8 (expression, msg);
  }
  if (function_name) {
    msg += " Function: ";
    Nirvana::append_utf8 (function_name, msg);
  }
  std::string file;
  if (file_name)
    Nirvana::append_utf8 (file_name, file);
  Nirvana::the_system->debug_event (Nirvana::System::DebugEvent::DEBUG_ERROR, msg, file, line_number);
}

extern "C" void __cdecl _invalid_parameter_noinfo ()
{
  _invalid_parameter (nullptr, nullptr, nullptr, 0, 0);
}

// This is used by inline code in the C++ Standard Library and the SafeInt
// library.  Because it is __declspec(noreturn), the compiler can better
// optimize use of the invalid parameter handler for inline code.
extern "C" __declspec(noreturn) void __cdecl _invalid_parameter_noinfo_noreturn ()
{
  _invalid_parameter (nullptr, nullptr, nullptr, 0, 0);
  abort ();
}
