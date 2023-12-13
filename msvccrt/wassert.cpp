#include <Nirvana/Nirvana.h>
#include <Nirvana/string_conv.h>
#include <assert.h>

extern "C" void Nirvana_assertion_failed (const char* msg, const char* file_name, int line_number);

extern "C" void __cdecl _wassert (
  _In_z_ wchar_t const* _Message,
  _In_z_ wchar_t const* _File,
  _In_   unsigned       _Line)
{
  std::string msg;
  std::string file;
  Nirvana::append_utf8 (_Message, msg);
  Nirvana::append_utf8 (_File, file);
  Nirvana_assertion_failed (msg.c_str (), file.c_str (), _Line);
}
