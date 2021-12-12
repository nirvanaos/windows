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
#ifndef NIRVANA_CORE_WINDOWS_UTF8_TO_UCS16_H_
#define NIRVANA_CORE_WINDOWS_UTF8_TO_UCS16_H_

#include <Nirvana/Nirvana.h>
#include <Heap.h>

namespace Nirvana {
namespace Core {
namespace Windows {

#if WCHAR_MAX == 0xffff
typedef wchar_t WinWChar;
#define WINWCS(s) L##s
#else
typedef char16_t WinWChar;
#define WINWCS(s) u##s
#endif

template <class A1, class A2> inline
void utf8_to_ucs16 (
	const std::basic_string <char, std::char_traits <char>, A1>& utf8,
	std::basic_string <WinWChar, std::char_traits <WinWChar>, A2>& ucs16)
{
	ucs16.resize (utf8.size ());
	ucs16.resize (utf8_to_ucs16 (utf8.data (), utf8.size (), &*ucs16.begin ()));
}

size_t utf8_to_ucs16 (const char* utf8, size_t len, WinWChar* ucs16);

typedef std::basic_string <WinWChar, std::char_traits <WinWChar>, CoreAllocator <WinWChar> > CoreStringW;

}
}
}

#endif
