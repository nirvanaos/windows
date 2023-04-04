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

#include <StaticallyAllocated.h>
#include "LockableHandle.h"
#include "../Port/Memory.h"

typedef struct _MEMORY_BASIC_INFORMATION64 MEMORY_BASIC_INFORMATION64;

namespace Nirvana {
namespace Core {
/// Namespace Windows contains internal implementation details of the Windows host.
/// This namespace is not visible to the nirvana Core code.
namespace Windows {

/// Memory block (64K) information.
struct BlockInfo
{
	// Currently we are using only one field. But we create structure for possible future extensions.
	LockableHandle mapping;
};

/// Logical address space of some Windows process.
template <bool x64>
class AddressSpace
{
	AddressSpace (const AddressSpace&) = delete;
	AddressSpace& operator = (const AddressSpace&) = delete;
public:
	AddressSpace (uint32_t process_id, HANDLE process_handle);
	AddressSpace () NIRVANA_NOEXCEPT;
	bool initialize (uint32_t process_id, HANDLE process_handle) NIRVANA_NOEXCEPT;

	~AddressSpace () NIRVANA_NOEXCEPT;

	HANDLE process () const
	{
		return process_;
	}

	static const size_t ADDRESS_SIZE = x64 ? 8 : 4;
	typedef std::conditional_t <x64, uint64_t, uint32_t> Size;
	typedef std::conditional_t <x64, int64_t, int32_t> SSize;
	typedef std::conditional_t <sizeof (void*) == ADDRESS_SIZE, uint8_t*, Size> Address;

	Address end () const NIRVANA_NOEXCEPT
	{
		return (Address)(directory_size_ * (Size)Port::Memory::ALLOCATION_UNIT);
	}

	BlockInfo& block (Address address)
	{
		BlockInfo* p = block_ptr (address, true);
		assert (p);
		return *p;
	}

	BlockInfo* allocated_block (Address address) NIRVANA_NOEXCEPT
	{
		BlockInfo* p = block_ptr (address, false);
		if (p && !p->mapping)
			p = nullptr;
		return p;
	}

	class Block
	{
	public:
		Block (AddressSpace& space, Address address, bool exclusive = false);
		~Block ();

		Address address () const
		{
			return address_;
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
			return ((HANDLE)(intptr_t)-1) == mapping_;
		}

		/// Lock the block exclusive
		/// \returns `true` if the block was not exclusive locked before
		bool exclusive_lock ();

		bool exclusive_locked () const
		{
			return exclusive_;
		}

		/// Virtual copy from other block
		/// Source block must be prepared for share
		void copy (Port::Memory::Block& src, size_t offset, size_t size, uint32_t copied_pages_state);

		/// Unmap the block
		void unmap ();

	protected:
		friend class Port::Memory;

		void invalidate_state ()
		{
			state_ = State::PAGE_STATE_UNKNOWN;
		}

		void map (HANDLE mapping_map, HANDLE mapping_store);

		void map_copy (HANDLE src_mapping);

		void protect (size_t offset, size_t size, uint32_t protection);

	private:
		static BlockInfo& check_block (BlockInfo* info)
		{
			if (!info)
				throw_BAD_PARAM ();
			return *info;
		}

	private:
		AddressSpace& space_;
		Address address_;
		BlockInfo& info_;
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

	Address reserve (Address dst, size_t& size, unsigned flags);
	void release (Address ptr, size_t size);

#ifndef _WIN64
	typedef std::conditional_t <x64, MEMORY_BASIC_INFORMATION64, MEMORY_BASIC_INFORMATION> MBI;
#else
	typedef MEMORY_BASIC_INFORMATION MBI;
#endif

	void query (Address address, MBI& mbi) const;

	void check_allocated (Address ptr, size_t size);

	void close_mapping (HANDLE hm) const;

private:
	friend class Port::Memory;

	Address alloc (Address address, size_t size, uint32_t flags, uint32_t protection) const;
	bool free (Address address, Size size, uint32_t flags) const;
	Address map (HANDLE hm, Address address, size_t size, uint32_t flags) const;
	bool unmap (Address address, uint32_t flags) const;

	BlockInfo* block_ptr (Address address, bool commit);

private:
	HANDLE process_;
	HANDLE file_;
	HANDLE mapping_;
	Size directory_size_;
	union {
		BlockInfo* directory32_;
		BlockInfo** directory64_;
		void* directory_;
	};
	static const uint32_t SECOND_LEVEL_BLOCK = Port::Memory::ALLOCATION_UNIT / sizeof (BlockInfo);
};

extern StaticallyAllocated <AddressSpace <sizeof (void*) == 8> > local_address_space;

}
}
}

#endif
