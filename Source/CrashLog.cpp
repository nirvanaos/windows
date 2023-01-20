/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#include "CrashLog.h"
#include "app_data.h"
#include <Nirvana/Formatter.h>

namespace Nirvana {
namespace Core {
namespace Windows {

CrashLog::CrashLog () :
	handle_ (INVALID_HANDLE_VALUE)
{
	WCHAR path [MAX_PATH + 1];
	size_t cc = get_app_data_folder (path, std::size (path), WINWCS ("var\\log"), false);
	if (cc) {
		WCHAR* name = path + cc;
		*(name++) = L'\\';
		SYSTEMTIME t;
		GetSystemTime (&t);
		Nirvana::sprintf (name, path + std::size (path) - name,
			WINWCS ("crash%4u%02u%02u_%02u%02u%02u_%u.txt"),
			t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, GetCurrentProcessId ());
		handle_ = CreateFileW (path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	}
}

CrashLog::~CrashLog ()
{
	if (INVALID_HANDLE_VALUE != handle_)
		CloseHandle (handle_);
}

const CrashLog& CrashLog::operator << (const char* text) const noexcept
{
	write (text, strlen (text));
	return *this;
}

void CrashLog::write (const char* text, size_t len) const noexcept
{
	const char* line = text;
	const char* end = text + len;
	for (;;) {
		const char* eol = std::find (line, end, '\n');

		DWORD cb;
		WriteFile (handle_, line, (DWORD)(eol - line), &cb, nullptr);
		if (eol != end) {
			const char crlf [] = "\r\n";
			WriteFile (handle_, crlf, 2, &cb, nullptr);
			line = eol + 1;
		} else
			break;
	}
}

}
}
}
