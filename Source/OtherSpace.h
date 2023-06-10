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
#ifndef NIRVANA_ESIOP_WINDOWS_OTHERSPACE_H_
#define NIRVANA_ESIOP_WINDOWS_OTHERSPACE_H_
#pragma once

#include <ORB/ESIOP.h>
#include "AddressSpace.h"
#include <limits>
#include <algorithm>

namespace ESIOP {
namespace Windows {

template <bool x64>
class OtherSpace : private Nirvana::Core::Windows::AddressSpace <x64>
{
	typedef Nirvana::Core::Windows::AddressSpace <x64> Base;
	typedef typename Base::Address Address;
	typedef typename Base::Size Size;

	static const size_t ALLOCATION_GRANULARITY = Nirvana::Core::Port::Memory::ALLOCATION_UNIT;

public:
	OtherSpace (ProtDomainId process_id, HANDLE process_handle) :
		Base (process_id, process_handle)
	{}

	SharedMemPtr reserve (size_t& size);
	SharedMemPtr copy (SharedMemPtr reserved, void* src, size_t& size, unsigned flags);
	void release (SharedMemPtr p, size_t size);

	static void get_sizes (PlatformSizes& sizes) noexcept
	{
		sizes.allocation_unit = ALLOCATION_GRANULARITY;
		sizes.block_size = ALLOCATION_GRANULARITY;
		sizes.sizeof_pointer = sizeof (Address);
		sizes.sizeof_size = sizeof (Size);
		sizes.max_size = std::max (std::numeric_limits <size_t>::max (), (size_t)std::numeric_limits <Size>::max ());
	}

	static void* store_pointer (void* where, SharedMemPtr p) noexcept
	{
		Address* ptr = (Address*)where;
		*ptr = (Address)p;
		return ptr + 1;
	}

	static void* store_size (void* where, size_t size) noexcept
	{
		Size* ptr = (Size*)where;
		*ptr = (Size)size;
		return ptr + 1;
	}
};

}
}

#endif
