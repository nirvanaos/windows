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

namespace Nirvana {
namespace Core {
/// Namespace Windows contains internal implementation details of the Windows host.
/// This namespace is not visible to the nirvana Core code.
namespace Windows {

/// Logical address space of some Windows process.
class AddressSpace
{
	AddressSpace (const AddressSpace&) = delete;
	AddressSpace& operator = (const AddressSpace&) = delete;
public:
	AddressSpace (uint32_t process_id, HANDLE process_handle);
	~AddressSpace () NIRVANA_NOEXCEPT;

	HANDLE process () const
	{
		return process_;
	}

	void* end () const
	{
		return (void*)(directory_size_ * Port::Memory::ALLOCATION_UNIT);
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

		void* address () const
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

		void map (HANDLE mapping, uint32_t protection);

	private:
		static BlockInfo& check_block (BlockInfo* info)
		{
			if (!info)
				throw_BAD_PARAM ();
			return *info;
		}

	private:
		AddressSpace& space_;
		void* address_;
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

	void* reserve (void* dst, size_t& size, unsigned flags);
	void release (void* ptr, size_t size);

	void query (const void* address, MEMORY_BASIC_INFORMATION& mbi) const;

	void check_allocated (void* ptr, size_t size);

	static void initialize ();

	static void terminate () NIRVANA_NOEXCEPT
	{
		local_space_.destruct ();
	}

	static AddressSpace& local () NIRVANA_NOEXCEPT
	{
		return local_space_;
	}

private:
	friend class Port::Memory;

	void protect (void* address, size_t size, uint32_t protection) const;

	BlockInfo* block_no_commit (const void* address);

private:
	HANDLE process_;
	HANDLE mapping_;
#ifdef _WIN64
	static const size_t SECOND_LEVEL_BLOCK = Port::Memory::ALLOCATION_UNIT / sizeof (BlockInfo);
	BlockInfo** directory_;
#else
	BlockInfo* directory_;
#endif
	size_t directory_size_;

	static StaticallyAllocated <AddressSpace> local_space_;
};

}
}
}

#endif
