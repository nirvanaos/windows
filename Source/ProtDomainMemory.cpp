// Nirvana project
// Protection domain memory service over Win32 API

#include "../Port/ProtDomainMemory.h"
#include <BackOff.h>
#include <Core.h>
#include "RunnableImpl.h"

namespace Nirvana {
namespace Core {
namespace Port {

Windows::AddressSpace ProtDomainMemory::space_;

struct ProtDomainMemory::Block::Regions
{
	Region begin [PAGES_PER_BLOCK];
	Region* end;

	Regions () :
		end (begin)
	{}

	void add (void* ptr, SIZE_T size)
	{
		assert (end < begin + PAGES_PER_BLOCK);
		Region* p = end++;
		p->ptr = ptr;
		p->size = size;
	}
};

class ProtDomainMemory::Block::Remap :
	public Windows::RunnableImpl <ProtDomainMemory::Block::Remap>
{
public:
	Remap (Block* block) :
		block_ (block)
	{}

	void run ()
	{
		block_->remap ();
	}

private:
	Block* block_;
};

DWORD ProtDomainMemory::Block::commit (SIZE_T offset, SIZE_T size)
{ // This operation must be thread-safe.

	assert (offset + size <= ALLOCATION_GRANULARITY);

	DWORD ret = 0;	// Page state bits in committed region
	HANDLE old_mapping = mapping ();
	if (INVALID_HANDLE_VALUE == old_mapping) {
		HANDLE hm = new_mapping ();
		try {
			map (hm, AddressSpace::MAP_PRIVATE, true);
		} catch (...) {
			CloseHandle (hm);
			throw;
		}
	}

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
			if (bs.page_state_bits & PageState::MASK_MAY_BE_SHARED) {
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
					throw NO_MEMORY ();
				}
			}
		}
	}
	return ret;
}

bool ProtDomainMemory::Block::need_remap_to_share (SIZE_T offset, SIZE_T size)
{
	const State& st = state ();
	if (st.state != State::MAPPED)
		throw BAD_PARAM ();
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

void ProtDomainMemory::Block::prepare_to_share_no_remap (SIZE_T offset, SIZE_T size)
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
				SIZE_T size = (region_end - region_begin) * PAGE_SIZE;
				protect (ptr, size, PageState::RW_MAPPED_SHARED);
			}

			region_begin = region_end;
		} while (region_begin < block_end);
	}

	if (st.page_state_bits & PAGE_GUARD) {
		// We are sharing block at the top of the stack.
		// We have to commit all pages in the block.
		for (auto end = st.mapped.page_state + PAGES_PER_BLOCK, begin = st.mapped.page_state, p = end;;) {
			auto prev = p - 1;
			DWORD s;
			if (s = *prev) {
				if (p < end && !VirtualAlloc (address () + (p - begin) * PAGE_SIZE, (end - p) * PAGE_SIZE, MEM_COMMIT, s))
					throw NO_MEMORY ();
				break;
			}
			if ((p = prev) == begin)
				break;
		}
	}
}

void ProtDomainMemory::Block::remap ()
{
	HANDLE hm;

	// If this block is on the top of current stack, we must remap it in different fiber.
	if (address () <= (BYTE*)&hm && (BYTE*)&hm < address () + ALLOCATION_GRANULARITY) {
		run_in_neutral_context (&Remap (this));
		return;
	}

	hm = new_mapping ();
	try {
		BYTE* ptmp = (BYTE*)space_.map (hm, AddressSpace::MAP_PRIVATE);
		if (!ptmp)
			throw NO_MEMORY ();
		assert (hm == space_.allocated_block (ptmp)->mapping);

		Regions read_only, read_write;
		try {
			auto page_state = state ().mapped.page_state;
			auto region_begin = page_state, block_end = page_state + PAGES_PER_BLOCK;
			do {
				auto region_end = region_begin;
				if (PageState::MASK_ACCESS & *region_begin) {
					do
						++region_end;
					while (region_end < block_end && (PageState::MASK_ACCESS & *region_end));

					SIZE_T offset = (region_begin - page_state) * PAGE_SIZE;
					LONG_PTR* dst = (LONG_PTR*)(ptmp + offset);
					SIZE_T size = (region_end - region_begin) * PAGE_SIZE;
					if (!VirtualAlloc (dst, size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE))
						throw NO_MEMORY ();
					const LONG_PTR* src = (LONG_PTR*)(address () + offset);
					real_copy (src, src + size / sizeof (LONG_PTR), dst);
					if (PageState::MASK_RO & *region_begin)
						read_only.add ((void*)src, size);
					else
						read_write.add ((void*)src, size);
				} else {
					do
						++region_end;
					while (region_end < block_end && !(PageState::MASK_ACCESS & *region_end));
				}

				region_begin = region_end;
			} while (region_begin < block_end);
		} catch (...) {
			space_.allocated_block (ptmp)->mapping = 0;
			verify (UnmapViewOfFile (ptmp));
			throw;
		}

		space_.allocated_block (ptmp)->mapping = 0;
		verify (UnmapViewOfFile (ptmp));

		// Change to new mapping
		map (hm, AddressSpace::MAP_PRIVATE);

		// Change protection.
		for (const Region* p = read_write.begin; p != read_write.end; ++p)
			protect (p->ptr, p->size, PageState::RW_MAPPED_PRIVATE);
		for (const Region* p = read_only.begin; p != read_only.end; ++p)
			protect (p->ptr, p->size, PageState::RO_MAPPED_PRIVATE);
	} catch (...) {
		CloseHandle (hm);
		throw;
	}
}

void ProtDomainMemory::Block::copy (void* src, SIZE_T size, LONG flags)
{
	assert (size);
	SIZE_T offset = (SIZE_T)src % ALLOCATION_GRANULARITY;
	assert (offset + size <= ALLOCATION_GRANULARITY);

	Block src_block (src);
	if ((offset || size < ALLOCATION_GRANULARITY) && INVALID_HANDLE_VALUE != mapping ()) {
		if (CompareObjectHandles (mapping (), src_block.mapping ())) {
			if (src_block.need_remap_to_share (offset, size)) {
				if (has_data_outside_of (offset, size, PageState::MASK_UNMAPPED)) {
					// Real copy
					copy (offset, size, src, flags);
					return;
				} else
					src_block.remap ();
			} else {
				SIZE_T page_off = offset % PAGE_SIZE;
				if (page_off) {
					SIZE_T page_tail = PAGE_SIZE - page_off;
					if (copy_page_part (src, page_tail, flags)) {
						offset += page_tail;
						size -= page_tail;
						assert (!(offset % PAGE_SIZE));
					}
				}
				page_off = (offset + size) % PAGE_SIZE;
				if (page_off && copy_page_part ((const BYTE*)src + size - page_off, page_off, flags)) {
					size -= page_off;
					assert (!((offset + size) % PAGE_SIZE));
				}
				if (!size)
					return;
			}
		} else if (has_data_outside_of (offset, size)) {
			// Real copy
			copy (offset, size, src, flags);
			return;
		} else if (src_block.need_remap_to_share (offset, size))
			src_block.remap ();
	} else if (src_block.need_remap_to_share (offset, size))
		src_block.remap ();

	if (!(flags & Memory::DECOMMIT))	// Memory::RELEASE includes flag DECOMMIT.
		src_block.prepare_to_share_no_remap (offset, size);
	AddressSpace::Block::copy (src_block, offset, size, flags);
}

void ProtDomainMemory::Block::copy (SIZE_T offset, SIZE_T size, const void* src, LONG flags)
{
	if (PageState::MASK_RO & commit (offset, size))
		change_protection (offset, size, Memory::READ_WRITE);
	real_copy ((const BYTE*)src, (const BYTE*)src + size, address () + offset);
	if (flags & Memory::READ_ONLY)
		change_protection (offset, size, Memory::READ_ONLY);
}

bool ProtDomainMemory::Block::copy_page_part (const void* src, SIZE_T size, LONG flags)
{
	SIZE_T offset = (SIZE_T)src % ALLOCATION_GRANULARITY;
	DWORD s = state ().mapped.page_state [offset / PAGE_SIZE];
	if (PageState::MASK_UNMAPPED & s) {
		BYTE* dst = address () + offset;
		if (PageState::MASK_RO & s)
			protect (dst, size, PageState::RW_UNMAPPED);
		real_copy ((const BYTE*)src, (const BYTE*)src + size, dst);
		if ((PageState::MASK_RO & s) && (flags & Memory::READ_ONLY))
			protect (dst, size, PageState::RO_UNMAPPED);
		return true;
	}
	return false;
}

DWORD ProtDomainMemory::commit_no_check (void* ptr, SIZE_T size)
{
	DWORD mask = 0;
	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end;) {
		Block block (p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		mask |= block.commit (p - block.address (), block_end - p);
		p = block_end;
	}
	return mask;
}

void ProtDomainMemory::prepare_to_share (void* src, SIZE_T size, LONG flags)
{
	if (!size)
		return;
	if (!src)
		throw BAD_PARAM ();

	for (BYTE* p = (BYTE*)src, *end = p + size; p < end;) {
		Block block (p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		block.prepare_to_share (p - block.address (), block_end - p, flags);
		p = block_end;
	}
}

void* ProtDomainMemory::allocate (void* dst, SIZE_T size, LONG flags)
{
	if (!size)
		throw BAD_PARAM ();

	if (flags & ~(Memory::RESERVED | Memory::EXACTLY | Memory::ZERO_INIT))
		throw INV_FLAG ();

	void* ret;
	try {
		if (!dst && size <= ALLOCATION_GRANULARITY && !(Memory::RESERVED & flags)) {
			// Optimization: quick allocate

			HANDLE mapping = new_mapping ();
			try {
				ret = space_.map (mapping, AddressSpace::MAP_PRIVATE);
			} catch (...) {
				CloseHandle (mapping);
				throw;
			}

			try {
				Block (ret).commit (0, size);
			} catch (...) {
				space_.release (ret, size);
				throw;
			}

		} else {

			if (!(ret = space_.reserve (size, flags, dst)))
				return 0;

			if (!(Memory::RESERVED & flags)) {
				try {
					commit_no_check (ret, size);
				} catch (...) {
					space_.release (ret, size);
					throw;
				}
			}
		}
	} catch (const NO_MEMORY&) {
		if (flags & Memory::EXACTLY)
			ret = 0;
		else
			throw;
	}
	return ret;
}

void ProtDomainMemory::commit (void* ptr, SIZE_T size)
{
	if (!size)
		return;

	if (!ptr)
		throw BAD_PARAM ();

	// Memory must be allocated.
	space_.check_allocated (ptr, size);

	commit_no_check (ptr, size);
}

void* ProtDomainMemory::copy (void* dst, void* src, SIZE_T size, LONG flags)
{
	if (!size)
		return dst;

	if (flags & ~(Memory::READ_ONLY | Memory::RELEASE | Memory::ALLOCATE | Memory::EXACTLY))
		throw INV_FLAG ();

	// Source range have to be committed.
	DWORD src_prot_mask = space_.check_committed (src, size);

	bool src_in_stack = is_current_stack (src), dst_in_stack = false;

	void* ret = 0;
	SIZE_T src_align = (SIZE_T)src % ALLOCATION_GRANULARITY;
	try {
		if (!dst && Memory::RELEASE != (flags & Memory::RELEASE) && !src_in_stack && round_up ((BYTE*)src + size, ALLOCATION_GRANULARITY) - (BYTE*)src <= ALLOCATION_GRANULARITY) {
			// Quick copy one block.
			Block block (src);
			block.prepare_to_share (src_align, size, flags);
			ret = space_.copy (block, src_align, size, flags);
		} else {
			Region allocated = {0, 0};
			if (!dst || (flags & Memory::ALLOCATE)) {
				if (dst) {
					if (dst == src) {
						if ((Memory::EXACTLY & flags) && Memory::RELEASE != (flags & Memory::RELEASE))
							return 0;
					} else {
						// Try reserve space exactly at dst.
						// Target region can overlap with source.
						allocated.ptr = dst;
						allocated.size = size;
						if (
							allocated.subtract (round_down (src, ALLOCATION_GRANULARITY), round_up ((BYTE*)src + size, ALLOCATION_GRANULARITY))
							&&
							space_.reserve (allocated.size, flags | Memory::EXACTLY, allocated.ptr)
							)
							ret = dst;
						else if (flags & Memory::EXACTLY)
							return 0;
					}
				}
				if (!ret) {
					if (Memory::RELEASE == (flags & Memory::RELEASE))
						ret = src;
					else {
						BYTE* res = (BYTE*)space_.reserve (size + src_align, flags);
						if (!res)
							return 0;
						ret = res + src_align;
						allocated.ptr = ret;
						allocated.size = size;
					}
				}
			} else {
				dst_in_stack = is_current_stack (dst);
				space_.check_allocated (dst, size);
				ret = dst;
			}

			assert (ret);

			if (ret == src) { // Special case - change protection.
				if ((Memory::ALLOCATE & flags) && Memory::RELEASE != (flags & Memory::RELEASE)) {
					dst = 0;
					if (flags & Memory::EXACTLY)
						return 0;
				} else {
					// Change protection
					if (src_prot_mask & ((flags & Memory::READ_ONLY) ? PageState::MASK_RW : PageState::MASK_RO))
						space_.change_protection (src, size, flags);
					return src;
				}
			}

			try {
				if (!src_in_stack && !dst_in_stack && (SIZE_T)ret % ALLOCATION_GRANULARITY == src_align) {
					// Share (regions may overlap).
					if (ret < src) {
						BYTE* pd = (BYTE*)ret, *end = pd + size;
						BYTE* ps = (BYTE*)src;
						if (end > src) {
							// Copy overlapped part with Memory::DECOMMIT.
							BYTE* first_part_end = round_up (end - ((BYTE*)src + size - end), ALLOCATION_GRANULARITY);
							assert (first_part_end < end);
							LONG first_part_flags = (flags & ~Memory::RELEASE) | Memory::DECOMMIT;
							while (pd < first_part_end) {
								Block block (pd);
								BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
								SIZE_T cb = block_end - pd;
								block.copy (ps, cb, first_part_flags);
								pd = block_end;
								ps += cb;
							}
						}
						while (pd < end) {
							Block block (pd);
							BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
							if (block_end > end)
								block_end = end;
							SIZE_T cb = block_end - pd;
							block.copy (ps, cb, flags);
							pd = block_end;
							ps += cb;
						}
					} else {
						BYTE* src_end = (BYTE*)src + size;
						BYTE* pd = (BYTE*)ret + size, *ps = (BYTE*)src + size;
						if (ret < src_end) {
							// Copy overlapped part with Memory::DECOMMIT.
							BYTE* first_part_begin = round_down ((BYTE*)ret + ((BYTE*)ret - (BYTE*)src), ALLOCATION_GRANULARITY);
							assert (first_part_begin > ret);
							LONG first_part_flags = (flags & ~Memory::RELEASE) | Memory::DECOMMIT;
							while (pd > first_part_begin) {
								BYTE* block_begin = round_down (pd - 1, ALLOCATION_GRANULARITY);
								Block block (block_begin);
								SIZE_T cb = pd - block_begin;
								ps -= cb;
								block.copy (ps, cb, first_part_flags);
								pd = block_begin;
							}
						}
						while (pd > ret) {
							BYTE* block_begin = round_down (pd - 1, ALLOCATION_GRANULARITY);
							if (block_begin < ret)
								block_begin = (BYTE*)ret;
							Block block (block_begin);
							SIZE_T cb = pd - block_begin;
							ps -= cb;
							block.copy (ps, cb, flags);
							pd = block_begin;
						}
					}
				} else {
					// Physical copy.
					DWORD state_bits = commit_no_check (ret, size);
					if (state_bits & PageState::MASK_RO)
						space_.change_protection (dst, size, Memory::READ_WRITE);
					real_move ((const BYTE*)src, (const BYTE*)src + size, (BYTE*)ret);
					if (flags & Memory::READ_ONLY)
						space_.change_protection (ret, size, Memory::READ_ONLY);

					if ((flags & Memory::DECOMMIT) && ret != src) {
						// Release or decommit source. Regions can overlap.
						Region reg = {src, size};
						if (flags & (Memory::RELEASE & ~Memory::DECOMMIT)) {
							if (reg.subtract (round_up (ret, ALLOCATION_GRANULARITY), round_down ((BYTE*)ret + size, ALLOCATION_GRANULARITY)))
								release (reg.ptr, reg.size);
						} else {
							if (reg.subtract (round_up (ret, PAGE_SIZE), round_down ((BYTE*)ret + size, PAGE_SIZE)))
								decommit (reg.ptr, reg.size);
						}
					}
				}
			} catch (...) {
				release (allocated.ptr, allocated.size);
				throw;
			}
		}
	} catch (const NO_MEMORY&) {
		if (Memory::EXACTLY & flags)
			ret = 0;
		else
			throw;
	}

	return ret;
}

SIZE_T ProtDomainMemory::query (const void* p, Memory::QueryParam q)
{
	{
		switch (q) {

		case Memory::ALLOCATION_SPACE_BEGIN:
			{
				SYSTEM_INFO sysinfo;
				GetSystemInfo (&sysinfo);
				return (SIZE_T)sysinfo.lpMinimumApplicationAddress;
			}

		case Memory::ALLOCATION_SPACE_END:
			return (SIZE_T)space_.end ();

		case Memory::ALLOCATION_UNIT:
		case Memory::SHARING_UNIT:
		case Memory::GRANULARITY:
		case Memory::SHARING_ASSOCIATIVITY:
		case Memory::OPTIMAL_COMMIT_UNIT:
			return ALLOCATION_GRANULARITY;

		case Memory::PROTECTION_UNIT:
		case Memory::COMMIT_UNIT:
			return PAGE_SIZE;

		case Memory::FLAGS:
			return
				Memory::ACCESS_CHECK |
				Memory::HARDWARE_PROTECTION |
				Memory::COPY_ON_WRITE |
				Memory::SPACE_RESERVATION;
		}

		throw BAD_PARAM ();
	}
}

LONG CALLBACK ProtDomainMemory::exception_filter (struct _EXCEPTION_POINTERS* pex)
{
	if (
		pex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
		&&
		pex->ExceptionRecord->NumberParameters >= 2
		&&
		!(pex->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
		) {
		void* address = (void*)pex->ExceptionRecord->ExceptionInformation [1];
		const AddressSpace::BlockInfo* block = space_.allocated_block (address);
		if (block) {
			if (INVALID_HANDLE_VALUE == block->mapping)
				throw MEM_NOT_COMMITTED ();
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (address, &mbi, sizeof (mbi)));
			if (MEM_MAPPED != mbi.Type) {
				Sleep (0);
				return EXCEPTION_CONTINUE_EXECUTION;
			} else if (!(mbi.Protect & PageState::MASK_ACCESS))
				throw MEM_NOT_COMMITTED ();
			else if (pex->ExceptionRecord->ExceptionInformation [0] && !(mbi.Protect & PageState::MASK_RW))
				throw NO_PERMISSION ();
			else
				return EXCEPTION_CONTINUE_EXECUTION;
		} else
			throw MEM_NOT_ALLOCATED ();
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

void ProtDomainMemory::se_translator (unsigned int, struct _EXCEPTION_POINTERS* pex)
{
	if (
		pex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
		&&
		pex->ExceptionRecord->NumberParameters >= 2
		&&
		!(pex->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
		) {
		void* address = (void*)pex->ExceptionRecord->ExceptionInformation [1];
		const AddressSpace::BlockInfo* block = space_.allocated_block (address);
		if (block) {
			if (INVALID_HANDLE_VALUE == block->mapping)
				throw MEM_NOT_COMMITTED ();
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (address, &mbi, sizeof (mbi)));
			if (MEM_MAPPED != mbi.Type) {
				Sleep (0);
				return;
			} else if (!(mbi.Protect & PageState::MASK_ACCESS))
				throw MEM_NOT_COMMITTED ();
			else if (pex->ExceptionRecord->ExceptionInformation [0] && !(mbi.Protect & PageState::MASK_RW))
				throw NO_PERMISSION ();
			else
				return;
		} else
			throw MEM_NOT_ALLOCATED ();
	}

	throw UNKNOWN ();
}

}
}
}
