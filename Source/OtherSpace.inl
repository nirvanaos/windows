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

	if (sizeof (size_t) > sizeof (Size)) {
		if (size > std::numeric_limits <Size>::max ())
			Nirvana::throw_IMP_LIMIT ();
	}

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
		Address d_p = dst, d_end = (Address)(d_p + size);
		size = (size_t)(Nirvana::round_up (d_end, (Size)ALLOCATION_GRANULARITY) - d_p);
		BYTE* s_p = (BYTE*)src;
		while (d_p < d_end) {
			size_t cb = (size_t)(std::min ((Address)(Nirvana::round_down (d_p, (Size)ALLOCATION_GRANULARITY) + ALLOCATION_GRANULARITY), d_end) - d_p);
			Memory::Block src_block (s_p, cb == ALLOCATION_GRANULARITY);
			typename Base::Block dst_block (*this, d_p, cb == ALLOCATION_GRANULARITY);
			dst_block.copy (src_block, (size_t)(s_p - src_block.address ()), cb, flags);
			d_p += (Size)cb;
			s_p += (Size)cb;
		}
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