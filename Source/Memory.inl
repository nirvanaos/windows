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

#include "../Port/Memory.h"
#include "AddressSpace.h"

namespace Nirvana {
namespace Core {
namespace Port {

inline
void Memory::protect (void* address, size_t size, uint32_t protection)
{
	//space_.protect (address, size, protection);
	DWORD old;
	verify (VirtualProtect (address, size, protection, &old));
}

inline
Windows::AddressSpace& Memory::space ()
{
	return space_;
}

class Memory::Block :
	public Core::Windows::AddressSpace::Block
{
public:
	Block (void* addr, bool exclusive = false) :
		Core::Windows::AddressSpace::Block (Memory::space (), addr, exclusive)
	{}

	DWORD commit (size_t offset, size_t size);
	bool need_remap_to_share (size_t offset, size_t size);

	void prepare_to_share (size_t offset, size_t size, unsigned flags)
	{
		if (need_remap_to_share (offset, size))
			if (exclusive_lock () && need_remap_to_share (offset, size))
				remap ();
		if (!(flags & Nirvana::Memory::SRC_DECOMMIT)) // Memory::SRC_RELEASE includes flag DECOMMIT.
			prepare_to_share_no_remap (offset, size);
	}

	void aligned_copy (void* src, size_t size, unsigned flags);
	void copy (size_t offset, size_t size, const void* src, unsigned flags);

	void decommit (size_t offset, size_t size);
	DWORD check_committed (size_t offset, size_t size);

	void change_protection (size_t offset, size_t size, unsigned flags);

	bool is_copy (Block& other, size_t offset, size_t size);

	bool is_private (size_t offset, size_t size);

private:
	struct Regions;

	struct CopyReadOnly
	{
		size_t offset;
		size_t size;
		const void* src;
	};

	void remap (const CopyReadOnly* copy_rgn = nullptr);
	void prepare_to_share_no_remap (size_t offset, size_t size);
};

struct Memory::Block::Regions
{
	Region begin [PAGES_PER_BLOCK];
	Region* end;

	Regions () :
		end (begin)
	{}

	void add (void* ptr, size_t size)
	{
		assert (end < begin + PAGES_PER_BLOCK);
		Region* p = end++;
		p->ptr = ptr;
		p->size = size;
	}
};

}
}
}

#endif

