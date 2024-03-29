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
#ifndef NIRVANA_CORE_WINDOWS_WINWCHAR_H_
#define NIRVANA_CORE_WINDOWS_WINWCHAR_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <UserAllocator.h>
#include <SharedAllocator.h>
#include <StringView.h>

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

/// Windows wide string.
typedef std::basic_string <WinWChar, std::char_traits <WinWChar> > StringW;

/// Windows wide string shared.
typedef std::basic_string <WinWChar, std::char_traits <WinWChar>, SharedAllocator <WinWChar> > SharedStringW;

}
}
}

#endif
