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
#ifndef NIRVANA_CORE_PORT_SYSTEMINFO_H_
#define NIRVANA_CORE_PORT_SYSTEMINFO_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Section.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// System information class.
class SystemInfo
{
public:
	static void initialize () NIRVANA_NOEXCEPT;

	static unsigned int hardware_concurrency () NIRVANA_NOEXCEPT
	{
		return hardware_concurrency_;
	}

	static bool get_OLF_section (Section& section) NIRVANA_NOEXCEPT;

	static const size_t SUPPORTED_PLATFORM_CNT =
#if HOST_PLATFORM == PLATFORM_X64
		2;
#elif HOST_PLATFORM == PLATFORM_I386
		1;
#else
#error Unsupported host.
#endif

	static const uint16_t supported_platforms_ [SUPPORTED_PLATFORM_CNT];

private:
	static unsigned int hardware_concurrency_;
};


}
}
}

#endif


