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
#ifndef NIRVANA_CORE_WINDOWS_ADDRESSSPACE_H_
#define NIRVANA_CORE_WINDOWS_ADDRESSSPACE_H_
#pragma once

#include "LockableHandle.h"
#include "win32.h"
#include <Psapi.h>

namespace Nirvana {
namespace Core {

namespace Port {
class Memory;
}

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
		READ_ONLY = PAGE_READONLY,
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

	ULONG_PTR is_mapped () const
	{
		return VirtualAttributes.Shared;
	}

	DWORD protection () const
	{
		return VirtualAttributes.Win32Protection;
	}

	// Returns page state
	DWORD state () const
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

ULONG handle_count (HANDLE h);

/// Logical address space of some Windows process.
class AddressSpace
{
	AddressSpace (const AddressSpace&) = delete;
	AddressSpace& operator = (const AddressSpace&) = delete;
public:
	AddressSpace (DWORD process_id, HANDLE process_handle);
	~AddressSpace () NIRVANA_NOEXCEPT;

	HANDLE process () const
	{
		return process_;
	}

	bool is_current_process () const
	{
		return GetCurrentProcess () == process_;
	}

	void* end () const
	{
		return (void*)(directory_size_ * ALLOCATION_GRANULARITY);
	}

	struct BlockInfo
	{
		// Currently we are using only one field. But we create structure for possible future extensions.
		LockableHandle mapping;
	};

	BlockInfo& block (const void* address);
	BlockInfo* allocated_block (const void* address);

	class Block
	{
	public:
		Block (AddressSpace& space, void* address, bool exclusive = false);
		~Block ();

		BYTE* address () const
		{
			return block_state_.address ();
		}

		HANDLE mapping () const
		{
			return mapping_;
		}

		void mapping (HANDLE hm)
		{
			assert (exclusive_);
			mapping_ = hm;
		}

		bool reserved () const
		{
			return INVALID_HANDLE_VALUE == mapping_;
		}

		/// Lock the block exclusive
		/// \returns `true` if the block was not exclusive locked before
		bool exclusive_lock ();

		bool exclusive_locked () const
		{
			return exclusive_;
		}

		/// Copy from other block
		/// Source block must be prepared for share
		void copy (Block& src, size_t offset, size_t size, unsigned flags);

		/// Unmap the block
		void unmap ();

		/// Obtain the block state
		const BlockState& state ();

	protected:
		friend class Port::Memory;

		void invalidate_state ()
		{
			state_ = State::PAGE_STATE_UNKNOWN;
		}

		void map (HANDLE mapping, DWORD protection);

		bool has_data (size_t offset, size_t size, DWORD mask = PageState::MASK_ACCESS);
		bool has_data_outside_of (size_t offset, size_t size, DWORD mask = PageState::MASK_ACCESS);

	private:
		static BlockInfo& check_block (BlockInfo* info)
		{
			if (!info)
				throw_BAD_PARAM ();
			return *info;
		}

		bool can_move (size_t offset, size_t size, unsigned flags)
		{
			bool move = false;
			if (flags & Memory::SRC_DECOMMIT) {
				if (flags & (Memory::SRC_RELEASE & ~Memory::SRC_DECOMMIT))
					move = true;
				else
					move = !has_data_outside_of (offset, size);
			}
			return move;
		}

	private:
		AddressSpace& space_;
		BlockInfo& info_;
		BlockState block_state_;
		HANDLE mapping_;

		enum class State
		{
			RESERVED, ///< Entire block is reserved
			MAPPED,   ///< Block is mapped, block_state_ is valid
			PAGE_STATE_UNKNOWN ///< Block is mapped, block_state_ is invalid
		}
		state_;

		bool exclusive_;
	};

	void* reserve (void* dst, size_t& size, unsigned flags);
	void release (void* ptr, size_t size);

	void query (const void* address, MEMORY_BASIC_INFORMATION& mbi) const
	{
		verify (VirtualQueryEx (process_, address, &mbi, sizeof (mbi)));
	}

	void check_allocated (void* ptr, size_t size);

private:
	friend class Port::Memory;

	void protect (void* address, size_t size, DWORD protection)
	{
		assert (!(protection & ~PageState::MASK_PROTECTION));
		assert (size && 0 == size % PAGE_SIZE);
		DWORD old;
		verify (VirtualProtectEx (process_, address, size, protection, &old));
	}

	BlockInfo* block_no_commit (const void* address);

private:
	HANDLE process_;
	HANDLE mapping_;
#ifdef _WIN64
	static const size_t SECOND_LEVEL_BLOCK = ALLOCATION_GRANULARITY / sizeof (BlockInfo);
	BlockInfo** directory_;
#else
	BlockInfo* directory_;
#endif
	size_t directory_size_;
};

}
}
}

#endif
