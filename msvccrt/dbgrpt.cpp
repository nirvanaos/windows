#include <Nirvana/Formatter.h>
#include <Nirvana/System.h>
#include <Nirvana/string_conv.h>
#include <crtdbg.h>

using namespace Nirvana;

extern "C" int __cdecl _CrtDbgReport (
	int         report_type,
	char const* file_name,
	int         line_number,
	char const* module_name,
	char const* format,
	...)
{
	std::string s;

	va_list arglist;
	va_start (arglist, format);
	append_format_v (s, format, arglist);
	va_end (arglist);

	g_system->debug_event ((System::DebugEvent)(report_type + 1), s, file_name, line_number);

	return 0;
}

extern "C" int __cdecl _CrtDbgReportW (
	int         report_type,
	wchar_t const* file_name,
	int         line_number,
	wchar_t const* module_name,
	wchar_t const* format,
	...)
{
	std::wstring s;
	if (module_name) {
		s = module_name;
		s += L": ";
	}

	va_list arglist;
	va_start (arglist, format);
	append_format_v (s, format, arglist);
	va_end (arglist);

	IDL::String msg, sfn;
	wide_to_utf8 (s, msg);
	wide_to_utf8 (file_name, sfn);

	g_system->debug_event ((System::DebugEvent)(report_type + 1), msg, sfn, line_number);

	return 0;
}
