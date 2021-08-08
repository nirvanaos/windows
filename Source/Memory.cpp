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
#include "Memory.inl"
#include <Nirvana/real_copy.h>

namespace Nirvana {
namespace Core {
namespace Port {

using namespace ::Nirvana::Core::Windows;
using namespace std;

int Memory::space_ [(sizeof (Windows::AddressSpace) + sizeof (int) - 1) / sizeof (int)];
void* Memory::handler_;

DWORD Memory::Block::commit (size_t offset, size_t size)
{ // This operation must be thread-safe.

	assert (offset + size <= ALLOCATION_GRANULARITY);

	if (INVALID_HANDLE_VALUE == mapping ())
		exclusive_lock ();
restart:
	DWORD ret = 0;	// Page state bits in committed region
	if (INVALID_HANDLE_VALUE == mapping ()) {
		HANDLE hm = new_mapping ();
		try {
			map (hm, AddressSpace::MAP_PRIVATE);
		} catch (...) {
			CloseHandle (hm);
			throw;
		}
		size_t begin = round_down (offset, PAGE_SIZE);
		size = round_up (offset + size, PAGE_SIZE) - begin;
		if (!VirtualAlloc (address () + begin, size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE))
			throw_NO_MEMORY ();
		ret = PageState::RW_MAPPED_PRIVATE;
	} else {
		if (size) {
			const State& bs = state ();
			Regions regions;	// Regions to commit.
			auto region_begin = bs.mapped.page_state + offset / PAGE_SIZE, state_end = bs.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
			do {
				auto region_end = region_begin;
				DWORD state = *region_begin;
				if (!(PageState::MASK_ACCESS & state)) {
					do {
						++region_end;
					} while (region_end < state_end && !(PageState::MASK_ACCESS & (state = *region_end)));

					regions.add (address () + (region_begin - bs.mapped.page_state) * PAGE_SIZE, (region_end - region_begin) * PAGE_SIZE);
				} else {
					do {
						ret |= state;
						++region_end;
					} while (region_end < state_end && (PageState::MASK_ACCESS & (state = *region_end)));
				}
				region_begin = region_end;
			} while (region_begin < state_end);

			if (regions.end != regions.begin) {

				// If the memory section is shared, we mustn't commit pages.
				// If the page is not committed and we commit it, it will become committed in another block.
				if (bs.page_state_bits & PageState::MASK_MAY_BE_SHARED) {
					if (exclusive_lock ())
						goto restart;
					remap ();
					ret = ((ret & PageState::MASK_RW) ? PageState::RW_MAPPED_PRIVATE : 0)
						| ((ret & PageState::MASK_RO) ? PageState::RO_MAPPED_PRIVATE : 0);
				}

				for (const Region* p = regions.begin; p != regions.end; ++p) {
					if (!VirtualAlloc (p->ptr, p->size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE)) {
						// Error, decommit back and throw the exception.
						while (p != regions.begin) {
							--p;
							protect (p->ptr, p->size, PageState::DECOMMITTED);
							verify (VirtualAlloc (p->ptr, p->size, MEM_RESET, PageState::DECOMMITTED));
						}
						throw_NO_MEMORY ();
					} else
						ret |= PageState::RW_MAPPED_PRIVATE;
				}

				invalidate_state ();
			}
		}
	}
	return ret;
}

DWORD Memory::Block::check_committed (size_t offset, size_t size)
{
	assert (offset + size <= ALLOCATION_GRANULARITY);

	const State& bs = state ();
	if (bs.state != State::MAPPED)
		throw_BAD_PARAM ();
	for (auto ps = bs.mapped.page_state + offset / PAGE_SIZE, end = bs.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE; ps < end; ++ps)
		if (!(PageState::MASK_ACCESS & *ps))
			throw_BAD_PARAM ();
	return bs.page_state_bits;
}

bool Memory::Block::need_remap_to_share (size_t offset, size_t size)
{
	const State& st = state ();
	if (st.state != State::MAPPED)
		throw_BAD_PARAM ();
	if (st.page_state_bits & PageState::MASK_UNMAPPED) {
		if (0 == offset && size == ALLOCATION_GRANULARITY)
			return true;
		else {
			for (auto ps = st.mapped.page_state + offset / PAGE_SIZE, end = st.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE; ps < end; ++ps) {
				if (PageState::MASK_UNMAPPED & *ps)
					return true;
			}
		}
	}
	return false;
}

void Memory::Block::prepare_to_share_no_remap (size_t offset, size_t size)
{
	assert (offset + size <= ALLOCATION_GRANULARITY);

	// Prepare pages
	const State& st = state ();
	assert (st.state == State::MAPPED);
	if (st.page_state_bits & PageState::RW_MAPPED_PRIVATE) {
		auto region_begin = st.mapped.page_state + offset / PAGE_SIZE, block_end = st.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
		do {
			auto region_end = region_begin;
			DWORD state = *region_begin;
			do
				++region_end;
			while (region_end < block_end && state == *region_end);

			if (PageState::RW_MAPPED_PRIVATE == state) {
				BYTE* ptr = address () + (region_begin - st.mapped.page_state) * PAGE_SIZE;
				size_t size = (region_end - region_begin) * PAGE_SIZE;
				protect (ptr, size, PageState::RW_MAPPED_SHARED);
			}

			region_begin = region_end;
		} while (region_begin < block_end);
	}
}

void Memory::Block::remap (const CopyReadOnly* copy_rgn)
{
	assert (exclusive_locked ());

	// Create a new memory section.
	HANDLE hm = new_mapping ();
	try {
		// Map memory section to temporary address.
		BYTE* ptmp = (BYTE*)MapViewOfFile (hm, FILE_MAP_WRITE, 0, 0, ALLOCATION_GRANULARITY);
		if (!ptmp)
			throw_NO_MEMORY ();

		auto ps = state ().mapped; // Create copy of page state
		const auto block_end = ps.page_state + PAGES_PER_BLOCK;

		// Copy data to the temporary address.
		try {
			auto region_begin = ps.page_state;
			DWORD* copy_begin = nullptr, * copy_end = nullptr;
			if (copy_rgn) {
				copy_begin = ps.page_state + (copy_rgn->offset + PAGE_SIZE - 1) / PAGE_SIZE;
				copy_end = ps.page_state + (copy_rgn->offset + copy_rgn->size) / PAGE_SIZE;
				fill (copy_begin, copy_end, 0);
			}
			do {
				auto region_end = region_begin;
				if (PageState::MASK_ACCESS & *region_begin) {
					DWORD access_mask = 0;
					do {
						access_mask |= *region_end;
						++region_end;
					} while (region_end < block_end && (PageState::MASK_ACCESS & *region_end));

					size_t offset = (region_begin - ps.page_state) * PAGE_SIZE;
					LONG_PTR* dst = (LONG_PTR*)(ptmp + offset);
					size_t size = (region_end - region_begin) * PAGE_SIZE;
					if (!VirtualAlloc (dst, size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE))
						throw_NO_MEMORY ();
					LONG_PTR* src = (LONG_PTR*)(address () + offset);
					
					// Temporary disable write access to save data consistency.
					if (access_mask & PageState::MASK_RW)
						protect (src, size, PAGE_READONLY);

					real_copy (src, src + size / sizeof (LONG_PTR), dst);
				} else {
					do
						++region_end;
					while (region_end < block_end && !(PageState::MASK_ACCESS & *region_end));
				}

				region_begin = region_end;
			} while (region_begin < block_end);

			if (copy_rgn) {
				if (copy_begin < copy_end) {
					fill (copy_begin, copy_end, PageState::RO_MAPPED_PRIVATE);
					size_t offset = (copy_begin - ps.page_state) * PAGE_SIZE;
					LONG_PTR* dst = (LONG_PTR*)(ptmp + offset);
					size_t size = (copy_end - copy_begin) * PAGE_SIZE;
					if (!VirtualAlloc (dst, size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE))
						throw_NO_MEMORY ();
				}
				const BYTE* src = (const BYTE*)copy_rgn->src;
				real_copy (src, src + copy_rgn->size, ptmp + copy_rgn->offset);
			}

		} catch (...) {
			verify (UnmapViewOfFile (ptmp));
			// Restore write access
			const DWORD* page_state = state ().mapped.page_state;
			for (const DWORD* ps = page_state, *end = ps + PAGES_PER_BLOCK; ps != end; ++ps) {
				if (PageState::MASK_RW & *ps)
					protect (address () + (ps - page_state) * PAGE_SIZE, PAGE_SIZE, *ps);
			}
			throw;
		}

		// Unmap memory section from the temporary address.
		verify (UnmapViewOfFile (ptmp));

		// Change this block mapping to the new.
		map (hm, AddressSpace::MAP_PRIVATE);

		// Change protection.
		auto region_begin = ps.page_state;
		do {
			auto region_end = region_begin;
			if (PageState::MASK_ACCESS & *region_begin) {
				DWORD state = *region_begin;
				do
					++region_end;
				while (region_end < block_end && state == *region_end);

				size_t offset = (region_begin - ps.page_state) * PAGE_SIZE;
				void* dst = address () + offset;
				size_t size = (region_end - region_begin) * PAGE_SIZE;
				// Even for RW pages we must change RW protection from shared (PAGE_EXECUTE_READWRITE) to private (PAGE_READWRITE).
				protect (dst, size, (PageState::MASK_RO & state) ? PageState::RO_MAPPED_PRIVATE : PageState::RW_MAPPED_PRIVATE);
			} else {
				do
					++region_end;
				while (region_end < block_end && !(PageState::MASK_ACCESS & *region_end));
			}

			region_begin = region_end;
		} while (region_begin < block_end);

	} catch (...) {
		CloseHandle (hm);
		throw;
	}
}

void Memory::Block::aligned_copy (void* src, size_t size, unsigned flags)
{
	// NOTE: Memory::SRC_DECOMMIT and Memory::SRC_RELEASE flags used only for optimozation.
	// We don't perform actual decommit or release here.
	assert (size);
	size_t offset = (uintptr_t)src % ALLOCATION_GRANULARITY;
	assert (offset + size <= ALLOCATION_GRANULARITY);

	Block src_block (src, size % ALLOCATION_GRANULARITY == 0);
	for (;;) {
		bool src_remap = false;
		if ((offset || size < ALLOCATION_GRANULARITY) && INVALID_HANDLE_VALUE != mapping ()) {
			if (CompareObjectHandles (mapping (), src_block.mapping ())) {
				if (src_block.need_remap_to_share (offset, size)) {
					if (has_data_outside_of (offset, size, PageState::MASK_UNMAPPED))
						goto fallback;
					else
						src_remap = true;
				} else if (!need_remap_to_share (offset, size))
					return; // If no unmapped pages at target region, we don't need to do anything.
				else if (has_data_outside_of (offset, size, PageState::MASK_UNMAPPED))
					goto fallback;
			} else if (has_data_outside_of (offset, size))
				goto fallback;
			else
				src_remap = src_block.need_remap_to_share (offset, size);
		} else
			src_remap = src_block.need_remap_to_share (offset, size);

		// Virtual copy is possible.
		if (src_remap) {
			exclusive_lock ();
			if (src_block.exclusive_lock ())
				continue;
			else
				src_block.remap ();
		}

		if (!exclusive_lock ())
			break;
	}

	// Virtual copy.
	if (!(flags & Nirvana::Memory::SRC_DECOMMIT))	// Memory::SRC_RELEASE includes flag DECOMMIT.
		src_block.prepare_to_share_no_remap (offset, size);
	AddressSpace::Block::copy (src_block, offset, size, flags);
	return;

fallback:
	// Real copy
	commit (offset, size);
	copy (offset, size, src, flags);
}

void Memory::Block::copy (size_t offset, size_t size, const void* src, unsigned flags)
{
	assert (size);
	assert (offset + size <= ALLOCATION_GRANULARITY);

repeat:
	DWORD page_state = check_committed (offset, size);
	if (PageState::MASK_RO & page_state) {
		// Some target pages are read-only
		if (flags & Nirvana::Memory::READ_ONLY) {
			if (PageState::MASK_UNMAPPED & page_state) {
				if (exclusive_lock ())
					goto repeat;
				// Remap
				CopyReadOnly cr = { offset, size, src };
				remap (&cr);
			} else {
				// Map memory section to temporary address.
				BYTE* ptmp = (BYTE*)MapViewOfFile (mapping (), FILE_MAP_WRITE, 0, 0, ALLOCATION_GRANULARITY);
				if (!ptmp)
					throw_NO_MEMORY ();
				real_copy ((const BYTE*)src, (const BYTE*)src + size, ptmp + offset);
				verify (UnmapViewOfFile (ptmp));
				if (PageState::MASK_RW & page_state)
					change_protection (offset, size, Nirvana::Memory::READ_ONLY);
			}
		} else {
			change_protection (offset, size, Nirvana::Memory::READ_WRITE);
			real_copy ((const BYTE*)src, (const BYTE*)src + size, address () + offset);
		}
	} else {
		real_copy ((const BYTE*)src, (const BYTE*)src + size, address () + offset);
		if (flags & Nirvana::Memory::READ_ONLY)
			change_protection (offset, size, Nirvana::Memory::READ_ONLY);
	}
}

void Memory::Block::decommit (size_t offset, size_t size)
{
	offset = round_up (offset, PAGE_SIZE);
	size_t offset_end = round_down (offset + size, PAGE_SIZE);
	assert (offset_end <= ALLOCATION_GRANULARITY);
	if (offset < offset_end) {
		if (!offset && offset_end == ALLOCATION_GRANULARITY)
			unmap ();
		else if (state ().state == State::MAPPED) {
			bool can_unmap = true;
			auto page_state = state ().mapped.page_state;
			if (offset) {
				for (auto ps = page_state, end = page_state + offset / PAGE_SIZE; ps < end; ++ps) {
					if (PageState::MASK_ACCESS & *ps) {
						can_unmap = false;
						break;
					}
				}
			}
			if (can_unmap && offset_end < ALLOCATION_GRANULARITY) {
				for (auto ps = page_state + offset_end / PAGE_SIZE, end = page_state + PAGES_PER_BLOCK; ps < end; ++ps) {
					if (PageState::MASK_ACCESS & *ps) {
						can_unmap = false;
						break;
					}
				}
			}

			if (can_unmap)
				unmap ();
			else {
				// Decommit pages. We can't use VirtualFree and MEM_DECOMMIT with mapped memory.
				space ().protect (address () + offset, offset_end - offset, PageState::DECOMMITTED | PAGE_REVERT_TO_FILE_MAP);

				// Discard private pages.
				auto region_begin = page_state + offset / PAGE_SIZE, state_end = page_state + offset_end / PAGE_SIZE;
				do {
					auto region_end = region_begin;
					if (!((PageState::MASK_MAY_BE_SHARED | PageState::DECOMMITTED) & *region_begin)) {
						do
							++region_end;
						while (region_end < state_end && !((PageState::MASK_MAY_BE_SHARED | PageState::DECOMMITTED) & *region_end));

						BYTE* ptr = address () + (region_begin - page_state) * PAGE_SIZE;
						size_t size = (region_end - region_begin) * PAGE_SIZE;
						verify (VirtualAlloc2 (space ().process (), ptr, size, MEM_RESET, PageState::DECOMMITTED, nullptr, 0));
					} else {
						do
							++region_end;
						while (region_end < state_end && ((PageState::MASK_MAY_BE_SHARED | PageState::DECOMMITTED) & *region_end));
					}
					region_begin = region_end;
				} while (region_begin < state_end);

				// Invalidate block state.
				invalidate_state ();
			}
		}
	}
}

void Memory::Block::change_protection (size_t offset, size_t size, unsigned flags)
{
	size_t offset_end = offset + size;
	assert (offset_end <= ALLOCATION_GRANULARITY);
	assert (size);

	static const size_t STATES_CNT = 3;

	static const DWORD states_RW [STATES_CNT] = {
		PageState::RW_MAPPED_PRIVATE,
		PageState::RW_MAPPED_SHARED,
		PageState::RW_UNMAPPED
	};

	static const DWORD states_RO [STATES_CNT] = {
		PageState::RO_MAPPED_PRIVATE,
		PageState::RO_MAPPED_SHARED,
		PageState::RO_UNMAPPED
	};

	DWORD protect_mask;
	const DWORD* states_src;
	const DWORD* states_dst;

	if (flags & Nirvana::Memory::READ_ONLY) {
		protect_mask = PageState::MASK_RO;
		states_src = states_RW;
		states_dst = states_RO;
		offset = round_up (offset, PAGE_SIZE);
		offset_end = round_down (offset_end, PAGE_SIZE);
	} else {
		protect_mask = PageState::MASK_RW;
		states_src = states_RO;
		states_dst = states_RW;
		offset = round_down (offset, PAGE_SIZE);
		offset_end = round_up (offset_end, PAGE_SIZE);
	}

	const DWORD* page_state = state ().mapped.page_state;
	auto region_begin = page_state + offset / PAGE_SIZE, state_end = page_state + offset_end / PAGE_SIZE;
	while (region_begin < state_end) {
		auto region_end = region_begin;
		const DWORD state = *region_begin;
		do
			++region_end;
		while (region_end < state_end && state == *region_end);

		if (!(protect_mask & state)) {

			DWORD new_state = state;
			for (size_t i = 0; i < STATES_CNT; ++i) {
				if (states_src [i] == state) {
					new_state = states_dst [i];
					break;
				}
			}

			if (new_state != state) {
				BYTE* ptr = address () + (region_begin - page_state) * PAGE_SIZE;
				size_t size = (region_end - region_begin) * PAGE_SIZE;
				protect (ptr, size, new_state);
			}
		}

		region_begin = region_end;
	}
}

inline
bool Memory::Block::is_copy (Block& other, size_t offset, size_t size)
{
	const State& st = state (), & other_st = other.state ();
	if (st.state != State::MAPPED || other_st.state != State::MAPPED)
		return false;
	if (!CompareObjectHandles (mapping (), other.mapping ()))
		return false;
	size_t page_begin = offset / PAGE_SIZE;
	auto pst = st.mapped.page_state + page_begin;
	auto other_pst = other_st.mapped.page_state + page_begin;
	auto pst_end = st.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
	for (; pst != pst_end; ++pst, ++other_pst) {
		DWORD ps = *pst, other_ps = *other_pst;
		if ((ps | other_ps) & PageState::MASK_UNMAPPED)
			return false;
		else if (!(ps & PageState::MASK_ACCESS) || !(other_ps & PageState::MASK_ACCESS))
			return false;
	}
	return true;
}

inline void Memory::query (const void* address, MEMORY_BASIC_INFORMATION& mbi) NIRVANA_NOEXCEPT
{
	//space ().query (address, mbi);
	verify (VirtualQuery (address, &mbi, sizeof (mbi)));
}

HANDLE Memory::new_mapping ()
{
	HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_EXECUTE_READWRITE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, 0);
	if (!mapping)
		throw_NO_MEMORY ();
	return mapping;
}

uint32_t Memory::commit_no_check (void* ptr, size_t size, bool exclusive)
{
	uint32_t state_bits = 0; // Page states bit mask
	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end;) {
		Block block (p, exclusive);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		state_bits |= block.commit (p - block.address (), block_end - p);
		p = block_end;
	}
	return state_bits;
}

void Memory::prepare_to_share (void* src, size_t size, unsigned flags)
{
	if (!size)
		return;
	if (!src)
		throw_BAD_PARAM ();

	for (BYTE* p = (BYTE*)src, *end = p + size; p < end;) {
		Block block (p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		block.prepare_to_share (p - block.address (), block_end - p, flags);
		p = block_end;
	}
}

void Memory::change_protection (void* ptr, size_t size, unsigned flags)
{
	if (!size)
		return;
	if (!ptr)
		throw_BAD_PARAM ();

	BYTE* begin = (BYTE*)ptr;
	BYTE* end = begin + size;
	for (BYTE* p = begin; p < end;) {
		Block block (p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		block.change_protection (p - block.address (), block_end - p, flags);
		p = block_end;
	}
}

void* Memory::allocate (void* dst, size_t size, unsigned flags)
{
	if (!size)
		throw_BAD_PARAM ();

	if (flags & ~(Nirvana::Memory::RESERVED | Nirvana::Memory::EXACTLY | Nirvana::Memory::ZERO_INIT))
		throw_INV_FLAG ();

	void* ret;
	try {
		if (!(ret = space ().reserve (dst, size, flags)))
			return nullptr;

		if (!(Nirvana::Memory::RESERVED & flags)) {
			try {
				uint32_t prot_mask = commit_no_check (ret, size, true);
				assert (!(prot_mask & PageState::MASK_RO));
			} catch (...) {
				space ().release (ret, size);
				throw;
			}
		}
	} catch (const CORBA::NO_MEMORY&) {
		if (flags & Nirvana::Memory::EXACTLY)
			ret = nullptr;
		else
			throw;
	}
	return ret;
}

void Memory::release (void* dst, size_t size)
{
	space ().release (dst, size);
}

void Memory::commit (void* ptr, size_t size)
{
	if (!size)
		return;

	if (!ptr)
		throw_BAD_PARAM ();

	// Memory must be allocated.
	space ().check_allocated (ptr, size);

	uint32_t prot_mask = commit_no_check (ptr, size);
	if (prot_mask & PageState::MASK_RO)
		change_protection (ptr, size, Nirvana::Memory::READ_WRITE);
}

void Memory::decommit (void* ptr, size_t size)
{
	if (!size)
		return;

	// Memory must be allocated.
	space ().check_allocated (ptr, size);

	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end;) {
		Block block (p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		block.decommit (p - block.address (), block_end - p);
		p = block_end;
	}
}

inline
uint32_t Memory::check_committed (void* ptr, size_t size)
{
	uint32_t state_bits = 0;
	for (const BYTE* begin = (const BYTE*)ptr, *end = begin + size; begin < end;) {
		MEMORY_BASIC_INFORMATION mbi;
		query (begin, mbi);
		if (!(mbi.Protect & PageState::MASK_ACCESS))
			throw_BAD_PARAM ();
		state_bits |= mbi.Protect;
		begin = (const BYTE*)mbi.BaseAddress + mbi.RegionSize;
	}
	return state_bits;
}

void* Memory::copy (void* dst, void* src, size_t size, unsigned flags)
{
	if (!size)
		return dst;

	if (flags & ~(Nirvana::Memory::READ_ONLY | Nirvana::Memory::SRC_RELEASE | Nirvana::Memory::DST_ALLOCATE | Nirvana::Memory::EXACTLY))
		throw_INV_FLAG ();

	bool src_own = false, dst_own = false;

	// release_flags can be 0, Memory::SRC_RELEASE, Memory::SRC_DECOMMIT.
	unsigned release_flags = flags & Nirvana::Memory::SRC_RELEASE;

	// Source range have to be committed.
	uint32_t src_prot_mask;
	if (space ().allocated_block (src)) {
		src_prot_mask = 0;
		for (BYTE* p = (BYTE*)src, *end = p + size; p < end;) {
			Block block (p);
			BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
			if (block_end > end)
				block_end = end;
			src_prot_mask |= block.check_committed (p - block.address (), block_end - p);
			p = block_end;
		}
		src_own = true;
	} else {
		if (release_flags)
			throw_FREE_MEM (); // Can't release memory that is not own.
		src_prot_mask = check_committed (src, size);
	}

	uintptr_t src_align = (uintptr_t)src % ALLOCATION_GRANULARITY;
	try {
		Region allocated = {0, 0};
		if (!dst || (flags & Nirvana::Memory::DST_ALLOCATE)) {
			if (dst) {
				if (dst == src) {
					if ((Nirvana::Memory::EXACTLY & flags) && Nirvana::Memory::SRC_RELEASE != (flags & Nirvana::Memory::SRC_RELEASE))
						return nullptr;
					dst = nullptr;
				} else {
					// Try reserve space exactly at dst.
					// Target region can overlap with source.
					allocated.ptr = dst;
					allocated.size = size;
					allocated.subtract (round_down (src, ALLOCATION_GRANULARITY), round_up ((BYTE*)src + size, ALLOCATION_GRANULARITY));
					dst = space ().reserve (allocated.ptr, allocated.size, flags | Nirvana::Memory::EXACTLY);
					if (!dst) {
						if (flags & Nirvana::Memory::EXACTLY)
							return nullptr;
						allocated.size = 0;
					}
				}
			}
			if (!dst) {
				if (Nirvana::Memory::SRC_RELEASE == release_flags)
					dst = src; // Memory::SRC_RELEASE is specified, so we can use source block as destination
				else {
					size_t dst_align = src_own ? src_align : 0;
					size_t cb_res = size + dst_align;
					BYTE* res = (BYTE*)space ().reserve (nullptr, cb_res, flags);
					if (!res) {
						assert (flags & Nirvana::Memory::EXACTLY);
						return nullptr;
					}
					dst = res + dst_align;
					allocated.ptr = res;
					allocated.size = cb_res;
				}
			}
			assert (dst);
			dst_own = true;
		} else {
			if (space ().allocated_block (dst)) {
				space ().check_allocated (dst, size);
				dst_own = true;
			} else if ((dst != src) && !is_writable (dst, size))
				throw_NO_PERMISSION ();
		}

		assert (dst);

		if (dst == src) { // Special case - change protection.
			// We allow to change protection of the not-allocated memory.
			// This is necessary for core services like module load.
			if (src_prot_mask & ((flags & Nirvana::Memory::READ_ONLY) ? PageState::MASK_RW : PageState::MASK_RO)) {
				if (dst_own)
					change_protection (src, size, flags);
				else if ((uintptr_t)dst % PAGE_SIZE == 0)
					protect (src, round_up (size, PAGE_SIZE), (flags & Nirvana::Memory::READ_ONLY) ? PageState::RO_UNMAPPED : PageState::RW_UNMAPPED);
				else
					throw_BAD_PARAM ();
			}
			return src;
		}

		try {
			if (dst_own) {
				if (src_own && (uintptr_t)dst % ALLOCATION_GRANULARITY == src_align) {
					// Share (regions may overlap).
					if (dst < src) {
						BYTE* d_p = (BYTE*)dst, * d_end = d_p + size;
						BYTE* s_p = (BYTE*)src;
						if (d_end > src) {
							// Copy overlapped part with Memory::SRC_DECOMMIT.
							BYTE* overlapped_end = round_up (d_end - ((BYTE*)src + size - d_end), ALLOCATION_GRANULARITY);
							assert (overlapped_end < d_end);
							unsigned overlapped_flags = (flags & ~Nirvana::Memory::SRC_RELEASE) | Nirvana::Memory::SRC_DECOMMIT;
							while (d_p < overlapped_end) {
								size_t cb = round_down (d_p, ALLOCATION_GRANULARITY) + ALLOCATION_GRANULARITY - d_p;
								Block block (d_p, cb % ALLOCATION_GRANULARITY == 0);
								block.aligned_copy (s_p, cb, overlapped_flags);
								d_p += cb;
								s_p += cb;
							}
						}
						while (d_p < d_end) {
							size_t cb = min (round_down (d_p, ALLOCATION_GRANULARITY) + ALLOCATION_GRANULARITY, d_end) - d_p;
							Block block (d_p, cb % ALLOCATION_GRANULARITY == 0);
							block.aligned_copy (s_p, cb, flags);
							d_p += cb;
							s_p += cb;
						}
					} else {
						BYTE* s_end = (BYTE*)src + size;
						BYTE* d_p = (BYTE*)dst + size, * s_p = (BYTE*)src + size;
						if (dst < s_end) {
							// Copy overlapped part with Memory::SRC_DECOMMIT.
							BYTE* overlapped_begin = round_down ((BYTE*)dst + ((BYTE*)dst - (BYTE*)src), ALLOCATION_GRANULARITY);
							assert (overlapped_begin > dst);
							unsigned overlapped_flags = (flags & ~Nirvana::Memory::SRC_RELEASE) | Nirvana::Memory::SRC_DECOMMIT;
							while (d_p > overlapped_begin) {
								BYTE* block_begin = round_down (d_p - 1, ALLOCATION_GRANULARITY);
								size_t cb = d_p - block_begin;
								Block block (block_begin, cb % ALLOCATION_GRANULARITY == 0);
								s_p -= cb;
								block.aligned_copy (s_p, cb, overlapped_flags);
								d_p = block_begin;
							}
						}
						while (d_p > dst) {
							BYTE* block_begin = round_down (d_p - 1, ALLOCATION_GRANULARITY);
							if (block_begin < dst)
								block_begin = (BYTE*)dst;
							size_t cb = d_p - block_begin;
							Block block (block_begin, cb % ALLOCATION_GRANULARITY == 0);
							s_p -= cb;
							block.aligned_copy (s_p, cb, flags);
							d_p = block_begin;
						}
					}
				} else {
					// Physical copy.
					uint32_t dst_prot_mask = commit_no_check (dst, size);
					if ((dst_prot_mask & PageState::MASK_RO) || (flags & Nirvana::Memory::READ_ONLY)) {
						if (dst < src) {
							BYTE* d_p = (BYTE*)dst, *d_end = d_p + size;
							BYTE* s_p = (BYTE*)src;
							while (d_p < d_end) {
								size_t cb = min (round_down (d_p, ALLOCATION_GRANULARITY) + ALLOCATION_GRANULARITY, d_end) - d_p;
								Block block (d_p, cb % ALLOCATION_GRANULARITY == 0);
								block.copy (d_p - block.address (), cb, s_p, flags);
								d_p += cb;
								s_p += cb;
							}
						} else {
							BYTE* d_p = (BYTE*)dst + size, * s_p = (BYTE*)src + size;
							while (d_p > dst) {
								BYTE* block_begin = round_down (d_p - 1, ALLOCATION_GRANULARITY);
								if (block_begin < dst)
									block_begin = (BYTE*)dst;
								size_t cb = d_p - block_begin;
								Block block (block_begin, cb % ALLOCATION_GRANULARITY == 0);
								s_p -= cb;
								block.copy (block_begin - block.address (), cb, s_p, flags);
								d_p = block_begin;
							}
						}
					} else
						real_move ((const BYTE*)src, (const BYTE*)src + size, (BYTE*)dst);
				}
			} else {
				// Physical copy not own memory.
				real_move ((const BYTE*)src, (const BYTE*)src + size, (BYTE*)dst);
			}

			if (flags & Nirvana::Memory::SRC_DECOMMIT) {
				// Release or decommit source. Regions can overlap.
				Region reg = {src, size};
				if (flags & (Nirvana::Memory::SRC_RELEASE & ~Nirvana::Memory::SRC_DECOMMIT)) {
					if (reg.subtract (round_up (dst, ALLOCATION_GRANULARITY), round_down ((BYTE*)dst + size, ALLOCATION_GRANULARITY)))
						release (reg.ptr, reg.size);
				} else {
					if (reg.subtract (round_up (dst, PAGE_SIZE), round_down ((BYTE*)dst + size, PAGE_SIZE)))
						decommit (reg.ptr, reg.size);
				}
			}
		} catch (...) {
			release (allocated.ptr, allocated.size);
			throw;
		}
	} catch (const CORBA::NO_MEMORY&) {
		if (Nirvana::Memory::EXACTLY & flags)
			dst = nullptr;
		else
			throw;
	}

	return dst;
}

uintptr_t Memory::query (const void* p, Nirvana::Memory::QueryParam q)
{
	switch (q) {

		case Nirvana::Memory::QueryParam::ALLOCATION_SPACE_BEGIN:
		{
			SYSTEM_INFO sysinfo;
			GetSystemInfo (&sysinfo);
			return (uintptr_t)sysinfo.lpMinimumApplicationAddress;
		}

		case Nirvana::Memory::QueryParam::ALLOCATION_SPACE_END:
			return (uintptr_t)space ().end ();

		case Nirvana::Memory::QueryParam::ALLOCATION_UNIT:
		case Nirvana::Memory::QueryParam::SHARING_UNIT:
		case Nirvana::Memory::QueryParam::GRANULARITY:
		case Nirvana::Memory::QueryParam::SHARING_ASSOCIATIVITY:
		case Nirvana::Memory::QueryParam::OPTIMAL_COMMIT_UNIT:
			return ALLOCATION_GRANULARITY;

		case Nirvana::Memory::QueryParam::PROTECTION_UNIT:
		case Nirvana::Memory::QueryParam::COMMIT_UNIT:
			return PAGE_SIZE;

		case Nirvana::Memory::QueryParam::FLAGS:
			return FLAGS;

		case Nirvana::Memory::QueryParam::MEMORY_STATE:
		{
			MEMORY_BASIC_INFORMATION mbi;
			query (p, mbi);
			switch (mbi.State) {
				case MEM_FREE:
					return (uintptr_t)Nirvana::Memory::MemoryState::MEM_NOT_ALLOCATED;
				case MEM_RESERVE:
					return (uintptr_t)Nirvana::Memory::MemoryState::MEM_RESERVED;
				default:
					if (mbi.Protect & PageState::MASK_RW)
						return (uintptr_t)Nirvana::Memory::MemoryState::MEM_READ_WRITE;
					else
						return (uintptr_t)Nirvana::Memory::MemoryState::MEM_READ_ONLY;
			}
		}

	}

	throw_BAD_PARAM ();
}

bool Memory::is_readable (const void* p, size_t size)
{
	for (const BYTE* begin = (const BYTE*)p, *end = begin + size; begin < end;) {
		MEMORY_BASIC_INFORMATION mbi;
		query (begin, mbi);
		if (!(mbi.Protect & PageState::MASK_ACCESS))
			return false;
		begin = (const BYTE*)mbi.BaseAddress + mbi.RegionSize;
	}
	return true;
}

bool Memory::is_private (const void* p, size_t size)
{
	for (const BYTE* begin = (const BYTE*)p, *end = begin + size; begin < end;) {
		MEMORY_BASIC_INFORMATION mbi;
		query (begin, mbi);
		if (mbi.Protect & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))
			return false;
		begin = (const BYTE*)mbi.BaseAddress + mbi.RegionSize;
	}
	return true;
}

bool Memory::is_writable (const void* p, size_t size)
{
	for (const BYTE* begin = (const BYTE*)p, *end = begin + size; begin < end;) {
		MEMORY_BASIC_INFORMATION mbi;
		query (begin, mbi);
		if (!(mbi.Protect & PageState::MASK_RW))
			return false;
		begin = (const BYTE*)mbi.BaseAddress + mbi.RegionSize;
	}
	return true;
}

bool Memory::is_copy (const void* p, const void* plocal, size_t size)
{
	if ((uintptr_t)p % ALLOCATION_GRANULARITY == (uintptr_t)plocal % ALLOCATION_GRANULARITY) {
		try {
			for (BYTE* begin1 = (BYTE*)p, *end1 = begin1 + size, *begin2 = (BYTE*)plocal; begin1 < end1;) {
				Block block1 (begin1);
				Block block2 (begin2);
				BYTE* block_end1 = block1.address () + ALLOCATION_GRANULARITY;
				if (block_end1 > end1)
					block_end1 = end1;
				if (!block1.is_copy (block2, begin1 - block1.address (), block_end1 - begin1))
					return false;
				begin1 = block_end1;
				begin2 = block2.address () + ALLOCATION_GRANULARITY;
			}
			return true;
		} catch (...) {
			return false;
		}
	} else
		return false;
}

long CALLBACK Memory::exception_filter (struct _EXCEPTION_POINTERS* pex)
{
	if (
		pex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
		&&
		pex->ExceptionRecord->NumberParameters >= 2
		&&
		!(pex->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
		) {
		void* address = (void*)pex->ExceptionRecord->ExceptionInformation [1];
		AddressSpace::BlockInfo* block = space ().allocated_block (address);
		if (block) {
			HANDLE mapping = block->mapping.lock ();
			if (INVALID_HANDLE_VALUE == mapping || nullptr == mapping) {
				block->mapping.unlock ();
				return EXCEPTION_EXECUTE_HANDLER;
			}
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (address, &mbi, sizeof (mbi)));
			block->mapping.unlock ();
			if (
				!(mbi.Protect & PageState::MASK_ACCESS)
				||
				(pex->ExceptionRecord->ExceptionInformation [0] && !(mbi.Protect & PageState::MASK_RW))
			)
				return EXCEPTION_EXECUTE_HANDLER;
			else
				return EXCEPTION_CONTINUE_EXECUTION;
		} else
			return EXCEPTION_EXECUTE_HANDLER;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

// Currently unused
void Memory::se_translator (unsigned int, struct _EXCEPTION_POINTERS* pex)
{
	if (
		pex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
		&&
		pex->ExceptionRecord->NumberParameters >= 2
		&&
		!(pex->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
		) {
		void* address = (void*)pex->ExceptionRecord->ExceptionInformation [1];
		AddressSpace::BlockInfo* block = space ().allocated_block (address);
		if (block) {
			HANDLE mapping = block->mapping.lock ();
			if (INVALID_HANDLE_VALUE == mapping || nullptr == mapping) {
				block->mapping.unlock ();
				throw_NO_PERMISSION ();
			}
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (address, &mbi, sizeof (mbi)));
			block->mapping.unlock ();
			if (
				!(mbi.Protect & PageState::MASK_ACCESS)
				||
				(pex->ExceptionRecord->ExceptionInformation [0] && !(mbi.Protect & PageState::MASK_RW))
			)
				throw_NO_PERMISSION ();
			else
				return;
		} else
			throw_NO_PERMISSION ();
	}

	throw_UNKNOWN ();
}

void Memory::initialize ()
{
	new (space_) AddressSpace (GetCurrentProcessId (), GetCurrentProcess ());
	handler_ = AddVectoredExceptionHandler (TRUE, &exception_filter);
}

void Memory::terminate () NIRVANA_NOEXCEPT
{
	space ().~AddressSpace ();
	RemoveVectoredExceptionHandler (handler_);
}

}
}
}
