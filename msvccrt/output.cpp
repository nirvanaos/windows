#include <Nirvana/Formatter.h>

using namespace Nirvana;

extern "C" int __cdecl __stdio_common_vsprintf_s (
  unsigned __int64 const options,
  char* const buffer,
  size_t           const buffer_count,
  char const* const format,
  _locale_t        const locale,
  va_list          const arglist
)
{
  Format <char> in (format);
  FormatOutBufSize <char> out (buffer, buffer_count);

  return Formatter::vformat (false, in, arglist, out);
}
