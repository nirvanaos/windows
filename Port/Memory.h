/// \file
/// Protection domain memory service over Win32 API.
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
#ifndef NIRVANA_CORE_PORT_MEMORY_H_
#define NIRVANA_CORE_PORT_MEMORY_H_
#pragma once

#include <CORBA/CORBA.h>
#include <Nirvana/Memory.h>

typedef void *HANDLE;
typedef struct _MEMORY_BASIC_INFORMATION MEMORY_BASIC_INFORMATION;

namespace Nirvana {
namespace Core {
namespace Port {

class Memory
{
	static const size_t PAGE_SIZE = 4096;
	static const size_t PAGES_PER_BLOCK = 16; // Windows allocate memory by 64K blocks
	static const size_t ALLOCATION_GRANULARITY = PAGE_SIZE * PAGES_PER_BLOCK;

public:
	static bool initialize () NIRVANA_NOEXCEPT;
	static void terminate () NIRVANA_NOEXCEPT;

	static const size_t ALLOCATION_UNIT = ALLOCATION_GRANULARITY;
	static const size_t SHARING_UNIT = PAGE_SIZE;
	static const size_t GRANULARITY = ALLOCATION_GRANULARITY;
	static const size_t SHARING_ASSOCIATIVITY = ALLOCATION_GRANULARITY;
	static const size_t OPTIMAL_COMMIT_UNIT = ALLOCATION_GRANULARITY;

	static const size_t FIXED_COMMIT_UNIT = PAGE_SIZE;
	static const size_t FIXED_PROTECTION_UNIT = PAGE_SIZE;

	static const unsigned FLAGS = 
		Nirvana::Memory::ACCESS_CHECK |
		Nirvana::Memory::HARDWARE_PROTECTION |
		Nirvana::Memory::COPY_ON_WRITE |
		Nirvana::Memory::SPACE_RESERVATION;

	// Memory::
	static void* allocate (void* dst, size_t& size, unsigned flags);
	static void release (void* dst, size_t size);
	static void commit (void* ptr, size_t size);
	static void decommit (void* ptr, size_t size);
	static void* copy (void* dst, void* src, size_t& size, unsigned flags);
	static bool is_private (const void* p, size_t size);
	static uintptr_t query (const void* p, Nirvana::Memory::QueryParam q);

	/// For usage in proxies.
	static void prepare_to_share (void* src, size_t size, unsigned flags);

	/// Used in test only
	static bool is_readable (const void* p, size_t size);

	/// Used in test only
	static bool is_writable (const void* p, size_t size);

	/// Used in test only
	static bool is_copy (const void* p1, const void* p2, size_t size);

	class Block;

private:
	struct Region
	{
		void* ptr;
		size_t size;

		size_t subtract (void* begin, void* end)
		{
			uint8_t* my_end = (uint8_t*)ptr + size;
			if (ptr < begin) {
				if (my_end >= end)
					size = 0;
				if (my_end > begin)
					size -= my_end - (uint8_t*)begin;
			} else if (end >= my_end)
				size = 0;
			else if (end > ptr) {
				size -= (uint8_t*)end - (uint8_t*)ptr;
				ptr = end;
			}
			return size;
		}
	};

private:
	static uint32_t commit_no_check (void* ptr, size_t size, bool exclusive = false);

	static void protect (void* address, size_t size, uint32_t protection);

	static void query (const void* address, MEMORY_BASIC_INFORMATION& mbi) NIRVANA_NOEXCEPT;

	static uint32_t check_committed (void* ptr, size_t size, uint32_t& type);

	static void change_protection (void* ptr, size_t size, unsigned flags);

	// Create new mapping
	static HANDLE new_mapping ();
};

}
}
}

#endif  // _WIN_MEMORY_H_
