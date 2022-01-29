#include <Nirvana/Nirvana.h>
#include <Windows.h>

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
  std::wstring s;
  if (file_name) {
    s += file_name;
    s += L'(';
    s += std::to_wstring (line_number);
    s += L"): ";
  }
  s += L"Invalid parameter";
  if (expression) {
    s += L' ';
    s += expression;
  }
  if (function_name) {
    s += L" Function: ";
    s += function_name;
  }
  s += L'\n';

  OutputDebugStringW (s.c_str ());
}
