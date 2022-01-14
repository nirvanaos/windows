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

namespace Nirvana {
namespace Core {

namespace Port {
class Memory;
}

namespace Windows {

///	<summary>
///	Page states for mapped block.
///	</summary>
///	<remarks>
///	<para>
///	We use "execute" protection to distinct private pages from shared pages.
///	Page states:
///	<list>
///		<item><term><c>0</c></term><description>Page not committed (entire block never was shared).</description></item>
///		<item><term><c>PAGE_NOACCESS</c></term><description>Decommitted.</description></item>
///		<item><term><c>PAGE_READWRITE</c></term><description>The page is mapped and never was shared.</description></item>
///		<item><term><c>PAGE_EXECUTE_WRITECOPY</c></term><description>The page is mapped and was shared.</description></item>
///		<item><term><c>PAGE_EXECUTE_READWRITE</c></term><description>The page is write-copyed (private, disconnected from mapping).</description></item>
///		<item><term><c>PAGE_READONLY</c></term><description>The read-only mapped page never was shared.</description></item>
///		<item><term><c>PAGE_EXECUTE</c></term><description>The read-only mapped page was shared.</description></item>
///		<item><term><c>PAGE_EXECUTE_READ</c></term><description>The page is not mapped. Page was write-copyed, than access was changed from PAGE_READWRITE to PAGE_READONLY.</description></item>
///	</list>
///	Note: "Page was shared" means that page has been shared at least once. Currently, page may be still shared or already not.
///	</para><para>
///	Page state changes.
///	<list>
///		<item>
///		Prepare to share:
///		<list>
///			<item><term><c>RW_MAPPED_PRIVATE</c>-><c>RW_MAPPED_SHARED</c></term></item>
///			<item><term><c>RO_MAPPED_PRIVATE</c>, <c>RW_MAPPED_SHARED</c>, <c>RO_MAPPED_SHARED</c>, <c>NOT_COMMITTED</c>, <c>DECOMMITTED</c></term><description>Unchanged.</description></item>
///			<item><term><c>RW_UNMAPPED</c>, <c>RO_UNMAPPED</c></term><description>We need to remap the block.</description></item>
///		</list>
///		</item><item>
///		Remap:
///		<list>
///			<item><term><c>RW_MAPPED_SHARED</c>, <c>RW_UNMAPPED</c>-><c>RW_MAPPED_PRIVATE</c></term>
///			<item><term><c>RO_MAPPED_SHARED</c>, <c>RO_UNMAPPED</c>-><c>RO_MAPPED_PRIVATE</c></term>
///		</list>
///		</item><item>
///		Write-protection:
///		<list>
///			<item><term><c>RW_MAPPED_PRIVATE</c><-><c>RO_MAPPED_PRIVATE</c></term>
///			<item><term><c>RW_MAPPED_SHARED</c><-><c>RO_MAPPED_SHARED</c></term>
///			<item><term><c>RW_UNMAPPED</c><-><c>RO_UNMAPPED</c></term>
///		</list>
///		</item>
///	</list>
///	</para>
///	</remarks>
class PageState
{
public:
	enum
	{
		/// Page not committed (entire block never was shared).
		NOT_COMMITTED = 0,
		/// Decommitted.
		DECOMMITTED = PAGE_NOACCESS,
		/// The page is mapped and never was shared.
		RW_MAPPED_PRIVATE = PAGE_READWRITE,
		/// The page is mapped and was shared.
		RW_MAPPED_SHARED = PAGE_EXECUTE_WRITECOPY,
		/// The page is write-copyed (private, disconnected from mapping).
		RW_UNMAPPED = PAGE_EXECUTE_READWRITE,
		/// The read-only mapped page never was shared.
		RO_MAPPED_PRIVATE = PAGE_READONLY,
		/// The read-only mapped page was shared.
		RO_MAPPED_SHARED = PAGE_EXECUTE,
		/// The page is not mapped. Page was write-copyed, than access was changed from <c>PAGE_READWRITE</c> to <c>PAGE_READONLY</c>.
		RO_UNMAPPED = PAGE_EXECUTE_READ,

		// Page state masks.
		MASK_RW = RW_MAPPED_PRIVATE | RW_MAPPED_SHARED | RW_UNMAPPED | PAGE_WRITECOPY,
		MASK_RO = RO_MAPPED_PRIVATE | RO_MAPPED_SHARED | RO_UNMAPPED,
		MASK_ACCESS = MASK_RW | MASK_RO,
		MASK_UNMAPPED = RW_UNMAPPED | RO_UNMAPPED,
		MASK_MAPPED = RW_MAPPED_PRIVATE | RW_MAPPED_SHARED | RO_MAPPED_PRIVATE | RO_MAPPED_SHARED,
		MASK_MAY_BE_SHARED = RW_MAPPED_SHARED | RO_MAPPED_SHARED | MASK_UNMAPPED | DECOMMITTED
	};
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

	enum MappingType
	{
		MAP_PRIVATE = PAGE_EXECUTE_READWRITE,
		MAP_SHARED = PAGE_EXECUTE_WRITECOPY
	};

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
		void unmap (HANDLE reserve = INVALID_HANDLE_VALUE);

		struct State
		{
			enum
			{
				INVALID = 0,
				RESERVED = MEM_RESERVE,
				MAPPED = MEM_MAPPED
			};

			DWORD state;

			/// <summary>
			/// The OR of all page_state.
			/// </summary>
			DWORD page_state_bits;

			union
			{
				/// <summary>
				/// Valid if block is mapped.
				/// </summary>
				struct PageState
				{
					DWORD page_state [PAGES_PER_BLOCK];
				}
				mapped;

				/// <summary>
				/// Valid if block is reserved.
				/// </summary>
				struct
				{
					BYTE* begin;
					BYTE* end;
				}
				reserved;
			};

			State () :
				state (INVALID)
			{}
		};

		const State& state ();

	protected:
		friend class Port::Memory;

		void invalidate_state ()
		{
			state_.state = State::INVALID;
		}

		void map (HANDLE mapping, MappingType protection);

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
		AddressSpace & space_;
		BYTE* address_;
		BlockInfo& info_;
		HANDLE mapping_;
		State state_;
		bool exclusive_;
	};

	void* reserve (void* dst, size_t size, unsigned flags);
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
