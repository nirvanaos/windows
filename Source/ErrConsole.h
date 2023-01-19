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
#ifndef NIRVANA_CORE_WINDOWS_ERRCONSOLE_H_
#define NIRVANA_CORE_WINDOWS_ERRCONSOLE_H_
#pragma once

#include <Nirvana/NirvanaBase.h>

typedef void* HANDLE;

namespace Nirvana {
namespace Core {
namespace Windows {

class ErrConsole
{
public:
	ErrConsole ();
	~ErrConsole ();

	static HANDLE attach () noexcept;

	const ErrConsole& operator << (const char* text) const noexcept;

	const ErrConsole& operator << (char c) const noexcept
	{
		write (&c, 1);
		return *this;
	}

private:
	void write (const char* text, size_t len) const noexcept;

private:
	static HANDLE handle_;
	static bool allocated_;
};

}
}
}

#endif
