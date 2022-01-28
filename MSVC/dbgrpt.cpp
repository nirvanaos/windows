#include <Nirvana/Formatter.h>
#include <crtdbg.h>
#include <Windows.h>

using namespace std;
using namespace Nirvana;

extern "C" int __cdecl _CrtDbgReport (
	int         report_type,
	char const* file_name,
	int         line_number,
	char const* module_name,
	char const* format,
	...)
{
	string s;
	if (module_name) {
		s = module_name;
		s += ": ";
	}

	if (file_name) {
		s += file_name;
		s += '(';
		s += to_string (line_number);
		s += "): ";
	}

	if (_CRT_ASSERT == report_type)
		s += "Assertion failed: ";
	else if (report_type)
		s += "ERROR: ";
	else
		s += "WARNING: ";

	CIn <char> in (format);
	COutContainer <string> out (s);

	va_list arglist;
	va_start (arglist, format);
	Formatter ().vformat (false, in, arglist, out);
	va_end (arglist);

	s += '\n';
	OutputDebugStringA (s.c_str ());

	return report_type ? 1 : 0;
}

extern "C" int __cdecl _CrtDbgReportW (
	int         report_type,
	WCHAR const* file_name,
	int         line_number,
	WCHAR const* module_name,
	WCHAR const* format,
	...)
{
	wstring s;
	if (module_name) {
		s = module_name;
		s += L": ";
	}

	if (file_name) {
		s += file_name;
		s += '(';
		s += to_wstring (line_number);
		s += L"): ";
	}

	if (_CRT_ASSERT == report_type)
		s += L"Assertion failed: ";
	else if (report_type)
		s += L"ERROR: ";
	else
		s += L"WARNING: ";

	CIn <WCHAR> in (format);
	COutContainer <wstring> out (s);

	va_list arglist;
	va_start (arglist, format);
	Formatter ().vformat (true, in, arglist, out);
	va_end (arglist);

	s += '\n';
	OutputDebugStringW (s.c_str ());

	return report_type ? 1 : 0;
}
