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
  Format <char> in (format);
  FormatOutBufSize <char> out (buffer, buffer_count);
  return Formatter::vformat (false, in, arglist, out);
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

namespace Nirvana {

class FileOut : public Formatter::COut
{
public:
  FileOut (FILE* f) :
    f_ (f)
  {}

  /// Put character to output.
  /// 
  /// \param c Character.
  virtual void put (int c) override
  {
    fputc (c, f_);
  }

private:
  FILE* f_;
};

}

extern "C" int __cdecl __stdio_common_vfprintf (
  unsigned __int64 const options,
  FILE* const stream,
  char const* const format,
  _locale_t        const locale,
  va_list          const arglist
)
{
  Format <char> in (format);
  FileOut out (stream);
  return Formatter::vformat (false, in, arglist, out);
}
