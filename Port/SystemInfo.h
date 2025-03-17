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
#include <Nirvana/platform.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// System information class.
class SystemInfo
{
public:
	static void initialize () noexcept;

	static void terminate () noexcept
	{}

	static unsigned int hardware_concurrency () noexcept
	{
		return hardware_concurrency_;
	}

	static const void* get_OLF_section (size_t& size) noexcept;

	static_assert (HOST_PLATFORM == PLATFORM_X64 || HOST_PLATFORM == PLATFORM_I386,
		"Unsupported host");

#if defined (__GNUG__) || defined (__clang__)
	static const size_t SUPPORTED_PLATFORM_CNT = 1;
#else
	static const size_t SUPPORTED_PLATFORM_CNT = (HOST_PLATFORM == PLATFORM_X64) ? 2 : 1;
#endif

	static const uint16_t* supported_platforms () noexcept;

	static const void* base_address () noexcept
	{
		return base_address_;
	}

private:
	static unsigned int hardware_concurrency_;
	static const void* base_address_;
};

}
}
}

#endif


