#include <Nirvana/Formatter.h>

using namespace Nirvana;

extern "C" int __cdecl __stdio_common_vsprintf (
  unsigned __int64 const options,
  char* const buffer,
  size_t           const buffer_count,
  char const* const format,
  _locale_t        const locale,
  va_list          const arglist
)
{
  WideInStrUTF8 in (format);
  WideOutBufUTF8 out (buffer, buffer + buffer_count);
  return Formatter::format (in, arglist, out);
}

extern "C" int __cdecl __stdio_common_vsprintf_s (
  unsigned __int64 const options,
  char* const buffer,
  size_t           const buffer_count,
  char const* const format,
  _locale_t        const locale,
  va_list          const arglist
)
{
  return __stdio_common_vsprintf (options, buffer, buffer_count, format, locale, arglist);
}

extern "C" int __cdecl __stdio_common_vsnprintf_s (
  unsigned __int64 const options,
  char* const buffer,
  size_t           const buffer_count,
  size_t           const max_count,
  char const* const format,
  _locale_t        const locale,
  va_list          const arglist
)
{
  return __stdio_common_vsprintf (options, buffer, buffer_count, format, locale, arglist);
}

extern "C" int __cdecl __stdio_common_vfprintf (
  unsigned __int64 const options,
  FILE* const stream,
  char const* const format,
  _locale_t        const locale,
  va_list          const arglist
)
{
  WideInStrUTF8 in (format);
  ByteOutFile out (stream);
  WideOutUTF8 wout (out);
  return Formatter::format (in, arglist, wout);
}
