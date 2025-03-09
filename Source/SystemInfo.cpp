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
#include "../Port/SystemInfo.h"
#include "../pe/PortableExecutable.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

unsigned int SystemInfo::hardware_concurrency_;
const void* SystemInfo::base_address_;

const uint16_t* SystemInfo::supported_platforms () noexcept
{
	static uint16_t platforms_x64 [] = {
		PLATFORM_X64,
		PLATFORM_I386
	};

	static uint16_t platforms_i386 [] = {
		PLATFORM_I386
	};

	if (HOST_PLATFORM == PLATFORM_X64)
		return platforms_x64;
	else
		return platforms_i386;
}

void SystemInfo::initialize () noexcept
{
	base_address_ = GetModuleHandleW (nullptr);
	::SYSTEM_INFO si;
	::GetSystemInfo (&si);
//	hardware_concurrency_ = si.dwNumberOfProcessors;
	hardware_concurrency_ = 1;
}

const void* SystemInfo::get_OLF_section (size_t& size) noexcept
{
	PortableExecutable pe (base_address_);
	return pe.find_OLF_section (size);
}

}
}
}
