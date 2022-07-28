/// \file
/// Protection domain (process) address space.
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

namespace Windows {

/// Page state for mapped block
/// 
///	We use "execute" protection to distinct private pages from shared pages.
///	Note: "Page was shared" means that page has been shared at least once. Currently, page may be still shared or already not.
///	Page state changes.
/// 
///		Prepare to share:
///			- #RW_MAPPED_PRIVATE -> #RW_MAPPED_SHARED
///			- #RO_MAPPED_PRIVATE, #RW_MAPPED_SHARED, #RO_MAPPED_SHARED, #NOT_COMMITTED, #DECOMMITTED: The page was not changed
///			- #RW_UNMAPPED, #RO_UNMAPPED: We need to remap the block.
/// 
///		Remap:
///			- #RW_MAPPED_SHARED, #RW_UNMAPPED -> #RW_MAPPED_PRIVAT
///			- #RO_MAPPED_SHARED, #RO_UNMAPPED -> #RO_MAPPED_PRIVATE
/// 
///		Write-protection:
///			#RW_MAPPED_PRIVATE <-> #RO_MAPPED_PRIVATE
///			#RW_MAPPED_SHARED <-> #RO_MAPPED_SHARED
///			#RW_UNMAPPED <-> #RO_UNMAPPED
/// 
struct PageState : public PSAPI_WORKING_SET_EX_INFORMATION
{
public:
	enum : DWORD
	{
		READ_WRITE_SHARED = PAGE_WRITECOPY,
		READ_WRITE_PRIVATE = PAGE_READWRITE,
		READ_ONLY = PAGE_EXECUTE_READ,

		// Page state masks.
		MASK_RW = PAGE_READWRITE | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY,
		MASK_RO = PAGE_READONLY | PAGE_EXECUTE | PAGE_EXECUTE_READ,
		MASK_ACCESS = MASK_RW | MASK_RO
	};

	ULONG_PTR is_mapped () const
	{
		return VirtualAttributes.Shared;
	}

	DWORD protection () const
	{
		assert (VirtualAttributes.Valid);
		return VirtualAttributes.Win32Protection;
	}
};

/// \brief Logical address space of some Windows process.
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
			return address_;
		}

		HANDLE mapping ()
		{
			return mapping_;
		}

		void mapping (HANDLE hm)
		{
			assert (exclusive_);
			mapping_ = hm;
		}

		bool exclusive_lock ();

		bool exclusive_locked () const
		{
			return exclusive_;
		}

		void copy (Block& src, size_t offset, size_t size, unsigned flags);
		void unmap ();

		struct State
		{
			// Walid if block is mapped
			struct PageState page_state [PAGES_PER_BLOCK];

			State (void* address) :
				state (INVALID)
			{
				BYTE* p = (BYTE*)address;
				PageState* ps = page_state;
				do {
					ps->VirtualAddress = p;
					p += PAGE_SIZE;
				} while (std::end (page_state) != ++ps);
			}

			enum BlockState
			{
				INVALID = 0,
				RESERVED = MEM_RESERVE,
				MAPPED = MEM_MAPPED
			}
			state;

			void* address ()
			{
				return page_state [0].VirtualAddress;
			}
		};

		const State& state ();

	protected:
		friend class Port::Memory;

		void invalidate_state ()
		{
			state_.state = State::INVALID;
		}

		void map (HANDLE mapping, DWORD protection);

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
		State state_;
		BlockInfo& info_;
		HANDLE mapping_;
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
