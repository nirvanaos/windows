/// \file
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
#ifndef NIRVANA_CORE_WINDOWS_DEBUGLOG_H_
#define NIRVANA_CORE_WINDOWS_DEBUGLOG_H_
#pragma once

#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class DebugLog
{
public:
	DebugLog ();
	~DebugLog ();

	const DebugLog& operator << (const char* text) const noexcept;
	const DebugLog& operator << (char c) const noexcept
	{
		write (&c, 1);
		return *this;
	}

	static HANDLE get_handle () noexcept;
	static void close_handle () noexcept;

private:
	static void write (const char* text, size_t len) noexcept;

private:
	static HANDLE handle_;
};

}
}
}

#endif
