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
using Nirvana::Core::Windows::PageState;

namespace ESIOP {
namespace Windows {

template <bool x64> inline
SharedMemPtr OtherSpace <x64>::reserve (size_t& size)
{
	return (SharedMemPtr)Base::reserve (0, size, 0);
}

template <bool x64> inline
SharedMemPtr OtherSpace <x64>::copy (SharedMemPtr reserved, void* src, size_t& size, unsigned flags)
{
	assert (!(flags & ~Nirvana::Memory::SRC_RELEASE));

	size_t size_in = size;
	if (!src || !size_in)
		Nirvana::throw_BAD_PARAM ();

	if (sizeof (size_t) > sizeof (Size)) {
		if (size_in > std::numeric_limits <Size>::max ())
			Nirvana::throw_IMP_LIMIT ();
	}

	void* tmp = nullptr;
	size_t tmp_size = 0;
	if (!Nirvana::Core::Windows::local_address_space->allocated_block ((uint8_t*)src)) {
		if (flags)
			Nirvana::throw_BAD_PARAM ();
		tmp_size = size;
		tmp = Memory::copy (nullptr, src, tmp_size, 0);
		src = tmp;
		flags = Nirvana::Memory::SRC_RELEASE;
	}

	DWORD copied_pages_state;
	if (flags) {
		copied_pages_state = PageState::READ_WRITE_PRIVATE;
	} else {
		copied_pages_state = PageState::READ_WRITE_SHARED;
	}

	Address dst = (Address)reserved;
	size_t src_align = (uintptr_t)src % Memory::ALLOCATION_UNIT;
	Address alloc_ptr = 0;
	size_t alloc_size = 0;
	if (dst) {
		if (src_align != (Size)dst % Memory::ALLOCATION_UNIT)
			Nirvana::throw_BAD_PARAM ();
		Base::check_allocated (dst, size_in);
	} else {
		alloc_size = src_align + size_in;
		alloc_ptr = Base::reserve (0, alloc_size, 0);
		dst = alloc_ptr + (Size)src_align;
	}
	try {
		Address d_p = dst, d_end = (Address)(d_p + size_in);
		size = (size_t)(Nirvana::round_up (d_end, (Size)ALLOCATION_GRANULARITY) - d_p);
		BYTE* s_p = (BYTE*)src;
		while (d_p < d_end) {
			size_t cb = (size_t)(std::min ((Address)(Nirvana::round_down (d_p, (Size)ALLOCATION_GRANULARITY) + ALLOCATION_GRANULARITY), d_end) - d_p);
			Memory::Block src_block (s_p, cb == ALLOCATION_GRANULARITY);
			size_t offset = (size_t)(s_p - src_block.address ());
			src_block.prepare_to_share (offset, cb, flags);
			typename Base::Block dst_block (*this, d_p, true);
			dst_block.copy (src_block, offset, cb, copied_pages_state);
			d_p += (Size)cb;
			s_p += (Size)cb;
		}
		switch (flags & Nirvana::Memory::SRC_RELEASE) {
		case Nirvana::Memory::SRC_RELEASE:
			Memory::release (src, size_in);
			break;
		case Nirvana::Memory::SRC_DECOMMIT: {
			BYTE* p = (BYTE*)src;
			BYTE* end = p + size_in;
			p = Nirvana::round_up (p, Memory::FIXED_COMMIT_UNIT);
			size_t cb = Nirvana::round_down (end, Memory::FIXED_COMMIT_UNIT) - p;
			Memory::decommit (p, cb);
		} break;
		}
	} catch (...) {
		if (alloc_size)
			Base::release (alloc_ptr, alloc_size);
		if (tmp)
			Memory::release (tmp, tmp_size);
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
