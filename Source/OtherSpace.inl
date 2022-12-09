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
#ifndef NIRVANA_ESIOP_WINDOWS_OTHERSPACE_INL_
#define NIRVANA_ESIOP_WINDOWS_OTHERSPACE_INL_
#pragma once

#include "OtherSpace.h"
#include "AddressSpace.inl"

using Nirvana::Core::Port::Memory;

namespace ESIOP {
namespace Windows {

template <bool x64> inline
SharedMemPtr OtherSpace <x64>::reserve (size_t size)
{
	return (SharedMemPtr)Base::reserve (0, size, 0);
}

template <bool x64> inline
SharedMemPtr OtherSpace <x64>::copy (SharedMemPtr reserved, void* src, size_t& size, bool release_src)
{
	if (!src || !size)
		Nirvana::throw_BAD_PARAM ();

	unsigned flags = release_src ? Nirvana::Memory::SRC_RELEASE : 0;
	Memory::prepare_to_share (src, size, flags);

	Address dst = (Address)reserved;
	size_t src_align = (uintptr_t)src % Memory::ALLOCATION_UNIT;
	Address alloc_ptr = 0;
	size_t alloc_size = 0;
	if (dst) {
		if (src_align != (Size)dst % Memory::ALLOCATION_UNIT)
			Nirvana::throw_BAD_PARAM ();
		Base::check_allocated (dst, size);
	} else {
		alloc_size = src_align + size;
		alloc_ptr = Base::reserve (0, alloc_size, 0);
		dst = alloc_ptr + (Size)src_align;
	}
	try {
		
	} catch (...) {
		if (alloc_size)
			Base::release (alloc_ptr, alloc_size);
		throw;
	}

	return (SharedMemPtr)dst;
}

template <bool x64> inline
void OtherSpace <x64>::release (SharedMemPtr p, size_t size)
{
	Base::release ((Address)p, size);
}

}
}

#endif
