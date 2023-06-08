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
#ifndef NIRVANA_CORE_WINDOWS_MEMORY_INL_
#define NIRVANA_CORE_WINDOWS_MEMORY_INL_
#pragma once

#include "AddressSpace.h"
#include "win32.h"
#include <Psapi.h>

namespace Nirvana {
namespace Core {

/// Namespace Windows contains internal implementation details of the Windows host.
/// This namespace is not visible to the nirvana Core code.
namespace Windows {

/// Page state for a mapped memory block
struct PageState : public PSAPI_WORKING_SET_EX_INFORMATION
{
public:
	enum : DWORD
	{
		/// @{ Page protection

		/// Page was committed but thed decommitted
		DECOMMITTED = PAGE_NOACCESS,
		READ_WRITE_SHARED = PAGE_WRITECOPY,
		READ_WRITE_PRIVATE = PAGE_READWRITE,
		READ_ONLY = PAGE_EXECUTE_READ,
		/// @}

		/// @{ Page state masks

		/// Page is mapped to a shared section
		MASK_MAPPED = 1 << 11,

		/// Page is detached from shared section due to copy-on-write
		MASK_UNMAPPED = 1 << 12,

		/// Page is not committed
		MASK_NOT_COMMITTED = 1 << 13,

		/// Read-write access
		MASK_RW = PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY,

		/// Read-only access
		MASK_RO = PAGE_READONLY | PAGE_EXECUTE | PAGE_EXECUTE_READ,

		/// Accessible memory
		MASK_ACCESS = MASK_RW | MASK_RO,

		/// No write access
		MASK_NO_WRITE = MASK_RO | MASK_NOT_COMMITTED | DECOMMITTED,

		/// @}

		/// Possible bits for VirtualProtect. Used in assertions.
		MASK_PROTECTION = MASK_ACCESS | DECOMMITTED | PAGE_REVERT_TO_FILE_MAP
	};

	ULONG_PTR is_mapped () const noexcept
	{
		return VirtualAttributes.Shared;
	}

	DWORD protection () const noexcept
	{
		return VirtualAttributes.Win32Protection;
	}

	bool is_committed () const noexcept
	{
		return VirtualAttributes.Valid || VirtualAttributes.Shared;
	}

	// Returns page state
	DWORD state () const noexcept
	{
		if (!VirtualAttributes.Valid && !VirtualAttributes.Shared)
			return MASK_NOT_COMMITTED;
		// Optimization: eliminate conditional operator
		// return (DWORD)VirtualAttributes.Win32Protection | (VirtualAttributes.Shared ? MASK_MAPPED : MASK_UNMAPPED);
		DWORD ret = (DWORD)(VirtualAttributes.Win32Protection | (VirtualAttributes.Shared << 11) | (!VirtualAttributes.Shared << 12));
		assert (((DWORD)VirtualAttributes.Win32Protection | (VirtualAttributes.Shared ? MASK_MAPPED : MASK_UNMAPPED)) == ret);
		return ret;
	}
};

// 64K block state
struct BlockState
{
	struct PageState page_state [PAGES_PER_BLOCK];

	BlockState (void* address)
	{
		BYTE* p = (BYTE*)address;
		PageState* ps = page_state;
		do {
			ps->VirtualAddress = p;
			p += PAGE_SIZE;
		} while (std::end (page_state) != ++ps);
	}

	void query (HANDLE process);

	BYTE* address () const
	{
		return (BYTE*)page_state [0].VirtualAddress;
	}
};

}

namespace Port {

inline
void Memory::protect (void* address, size_t size, uint32_t protection)
{
	assert (!(protection & ~Nirvana::Core::Windows::PageState::MASK_PROTECTION));
	assert (size && 0 == size % PAGE_SIZE);
	DWORD old;
	verify (VirtualProtect (address, size, protection, &old));
}

class Memory::Block :
	public Windows::AddressSpace <sizeof (void*) == 8>::Block
{
	typedef Windows::AddressSpace <sizeof (void*) == 8> Space;
	typedef Space::Block Base;
public:
	Block (void* addr, bool exclusive = false) :
		Base (Windows::local_address_space, (uint8_t*)addr, exclusive),
		block_state_ (address ())
	{}

	DWORD commit (size_t offset, size_t size);
	bool need_remap_to_share (size_t offset, size_t size);

	void prepare_to_share (size_t offset, size_t size, unsigned flags)
	{
		if (need_remap_to_share (offset, size)) {
			if (!exclusive_lock () || need_remap_to_share (offset, size))
				remap ();
		}
		if (!(flags & Nirvana::Memory::SRC_DECOMMIT)) // Memory::SRC_RELEASE includes flag DECOMMIT.
			prepare_to_share_no_remap (offset, size);
	}

	void copy_aligned (Block& src_block, void* src, size_t size, unsigned flags);
	void copy_unaligned (size_t offset, size_t size, const void* src, unsigned flags);

	void decommit (size_t offset, size_t size);
	DWORD check_committed (size_t offset, size_t size);

	void change_protection (size_t offset, size_t size, unsigned flags);

	bool is_copy (Block& other, size_t offset, size_t size);

	bool is_private (size_t offset, size_t size);

	/// Obtain the block state
	const Windows::BlockState& state ();

private:
	struct CopyReadOnly
	{
		size_t offset;
		size_t size;
		const void* src;
	};

	bool has_data (size_t offset, size_t size, uint32_t mask = Windows::PageState::MASK_ACCESS);
	bool has_data_outside_of (size_t offset, size_t size, uint32_t mask = Windows::PageState::MASK_ACCESS);
	void remap (const CopyReadOnly* copy_rgn = nullptr);
	void prepare_to_share_no_remap (size_t offset, size_t size);

	bool can_move (size_t offset, size_t size, unsigned flags)
	{
		bool move = false;
		if (flags & Nirvana::Memory::SRC_DECOMMIT) {
			if (flags & (Nirvana::Memory::SRC_RELEASE & ~Nirvana::Memory::SRC_DECOMMIT))
				move = true;
			else
				move = !has_data_outside_of (offset, size);
		}
		return move;
	}

	void adjust_protection (const DWORD page_protection [PAGES_PER_BLOCK]);

private:
	Windows::BlockState block_state_;
};

}
}
}

#endif

