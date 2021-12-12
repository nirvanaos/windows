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
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

size_t utf8_to_ucs16 (const char* utf8, size_t len, WinWChar* ucs16)
{
	size_t ret = MultiByteToWideChar (CP_UTF8, MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, utf8, len, ucs16, len);
	if (len && !ret)
		throw_CODESET_INCOMPATIBLE ();
	return ret;
}

}
}
}
