// Nirvana project
// Protection domain memory service over Win32 API

#include <eh.h>
#include "ProtDomainMemoryInternal.h"
#include <Nirvana/real_copy.h>
#include <CORBA/CORBA.h>

namespace Nirvana {
namespace Core {
namespace Port {

using namespace ::Nirvana::Core::Windows;

Windows::AddressSpace ProtDomainMemory::space_;

DWORD ProtDomainMemory::Block::commit (size_t offset, size_t size)
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

			// If the memory section is shared, we mustn't commit pages.
			// If the page is not committed and we commit it, it will become committed in another block.
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
					throw_NO_MEMORY ();
				}
			}
		}
	}
	return ret;
}

bool ProtDomainMemory::Block::need_remap_to_share (size_t offset, size_t size)
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

void ProtDomainMemory::Block::prepare_to_share_no_remap (size_t offset, size_t size)
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

	if (st.page_state_bits & PAGE_GUARD) {
		// We are sharing block at the top of the stack.
		// We have to commit all pages in the block.
		for (auto end = st.mapped.page_state + PAGES_PER_BLOCK, begin = st.mapped.page_state, p = end;;) {
			auto prev = p - 1;
			DWORD s;
			if (s = *prev) {
				if (p < end && !VirtualAlloc (address () + (p - begin) * PAGE_SIZE, (end - p) * PAGE_SIZE, MEM_COMMIT, s))
					throw_NO_MEMORY ();
				break;
			}
			if ((p = prev) == begin)
				break;
		}
	}
}

void ProtDomainMemory::Block::remap ()
{
	// Create a new memory section.
	HANDLE hm = new_mapping ();
	try {
		// Map memory section to temporary address.
		BYTE* ptmp = (BYTE*)MapViewOfFile (hm, FILE_MAP_WRITE, 0, 0, ALLOCATION_GRANULARITY);
		if (!ptmp)
			throw_NO_MEMORY ();

		// Copy data to the temporary address.
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

					size_t offset = (region_begin - page_state) * PAGE_SIZE;
					LONG_PTR* dst = (LONG_PTR*)(ptmp + offset);
					size_t size = (region_end - region_begin) * PAGE_SIZE;
					if (!VirtualAlloc (dst, size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE))
						throw_NO_MEMORY ();
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
			verify (UnmapViewOfFile (ptmp));
			throw;
		}

		// Unmap memory section from the temporary address.
		verify (UnmapViewOfFile (ptmp));

		// Change this block mapping to the new.
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

void ProtDomainMemory::Block::aligned_copy (void* src, size_t size, UWord flags)
{
	// NOTE: Memory::DECOMMIT and Memory::RELEASE flags used only for optimozation.
	// We don't perform actual decommit or release here.
	assert (size);
	size_t offset = (uintptr_t)src % ALLOCATION_GRANULARITY;
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
				
			} else if (!need_remap_to_share (offset, size))
				return; // If no unmapped pages at target region, we don't need to do anything.
			else if (has_data_outside_of (offset, size, PageState::MASK_UNMAPPED)) {
				// Real copy
				copy (offset, size, src, flags);
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

void ProtDomainMemory::Block::copy (size_t offset, size_t size, const void* src, UWord flags)
{
	assert (size);
	assert (offset + size <= ALLOCATION_GRANULARITY);

	DWORD page_state = check_committed (offset, size);
	if (PageState::MASK_RO & page_state) {
		// Some target pages are read-only
		if (flags & Memory::READ_ONLY) {
			// Map memory section to temporary address.
			BYTE* ptmp = (BYTE*)MapViewOfFile (mapping (), FILE_MAP_WRITE, 0, 0, ALLOCATION_GRANULARITY);
			if (!ptmp)
				throw_NO_MEMORY ();
			real_copy ((const BYTE*)src, (const BYTE*)src + size, ptmp + offset);
			verify (UnmapViewOfFile (ptmp));
			if (PageState::MASK_RW & page_state)
				change_protection (offset, size, Memory::READ_ONLY);
		} else {
			change_protection (offset, size, Memory::READ_WRITE);
			real_copy ((const BYTE*)src, (const BYTE*)src + size, address () + offset);
		}
	} else {
		real_copy ((const BYTE*)src, (const BYTE*)src + size, address () + offset);
		if (flags & Memory::READ_ONLY)
			change_protection (offset, size, Memory::READ_ONLY);
	}
}

void ProtDomainMemory::initialize ()
{
	space_.initialize ();
	//		SetUnhandledExceptionFilter (&exception_filter);
	_set_se_translator (&se_translator);
}

void ProtDomainMemory::terminate ()
{
	//		SetUnhandledExceptionFilter (0);
	space_.terminate ();
}

inline void ProtDomainMemory::query (const void* address, MEMORY_BASIC_INFORMATION& mbi)
{
	//space_.query (address, mbi);
	verify (VirtualQuery (address, &mbi, sizeof (mbi)));
}

HANDLE ProtDomainMemory::new_mapping ()
{
	HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_EXECUTE_READWRITE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, 0);
	if (!mapping)
		throw_NO_MEMORY ();
	return mapping;
}

uint32_t ProtDomainMemory::commit_no_check (void* ptr, size_t size)
{
	uint32_t state_bits = 0; // Page states bit mask
	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end;) {
		Block block (p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		state_bits |= block.commit (p - block.address (), block_end - p);
		p = block_end;
	}
	return state_bits;
}

void ProtDomainMemory::prepare_to_share (void* src, size_t size, UWord flags)
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

void* ProtDomainMemory::allocate (void* dst, size_t size, UWord flags)
{
	if (!size)
		throw_BAD_PARAM ();

	if (flags & ~(Memory::RESERVED | Memory::EXACTLY | Memory::ZERO_INIT))
		throw_INV_FLAG ();

	void* ret;
	try {
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
	} catch (const CORBA::NO_MEMORY&) {
		if (flags & Memory::EXACTLY)
			ret = 0;
		else
			throw;
	}
	return ret;
}

void ProtDomainMemory::release (void* dst, size_t size)
{
	space_.release (dst, size);
}

void ProtDomainMemory::commit (void* ptr, size_t size)
{
	if (!size)
		return;

	if (!ptr)
		throw_BAD_PARAM ();

	// Memory must be allocated.
	space_.check_allocated (ptr, size);

	uint32_t prot_mask = commit_no_check (ptr, size);
	if (prot_mask & PageState::MASK_RO)
		space_.change_protection (ptr, size, Memory::READ_WRITE);
}

void ProtDomainMemory::decommit (void* ptr, size_t size)
{
	space_.decommit (ptr, size);
}

uint32_t ProtDomainMemory::check_committed (void* ptr, size_t size)
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

void* ProtDomainMemory::copy (void* dst, void* src, size_t size, UWord flags)
{
	if (!size)
		return dst;

	if (flags & ~(Memory::READ_ONLY | Memory::RELEASE | Memory::ALLOCATE | Memory::EXACTLY))
		throw_INV_FLAG ();

	bool src_own = false, dst_own = false;

	// release_flags can be 0, Memory::RELEASE, Memory::DECOMMIT.
	UWord release_flags = flags & Memory::RELEASE;

	// Source range have to be committed.
	uint32_t src_prot_mask;
	if (space_.allocated_block (src)) {
		src_prot_mask = space_.check_committed (src, size);
		src_own = true;
	} else {
		if (release_flags)
			throw_BAD_PARAM (); // Can't release memory that is not own.
		src_prot_mask = check_committed (src, size);
	}

	uintptr_t src_align = (uintptr_t)src % ALLOCATION_GRANULARITY;
	try {
		Region allocated = {0, 0};
		if (!dst || (flags & Memory::ALLOCATE)) {
			if (dst) {
				if (dst == src) {
					if ((Memory::EXACTLY & flags) && Memory::RELEASE != (flags & Memory::RELEASE))
						return nullptr;
					dst = nullptr;
				} else {
					// Try reserve space exactly at dst.
					// Target region can overlap with source.
					allocated.ptr = dst;
					allocated.size = size;
					allocated.subtract (round_down (src, ALLOCATION_GRANULARITY), round_up ((BYTE*)src + size, ALLOCATION_GRANULARITY));
					dst = space_.reserve (allocated.size, flags | Memory::EXACTLY, allocated.ptr);
					if (!dst && (flags & Memory::EXACTLY))
						return nullptr;
				}
			}
			if (!dst) {
				if (Memory::RELEASE == release_flags)
					dst = src; // Memory::RELEASE is specified, so we can use source block as destination
				else {
					size_t dst_align = src_own ? src_align : 0;
					size_t cb_res = size + dst_align;
					BYTE* res = (BYTE*)space_.reserve (cb_res, flags);
					if (!res) {
						assert (flags & Memory::EXACTLY);
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
			if (space_.allocated_block (dst)) {
				space_.check_allocated (dst, size);
				dst_own = true;
			} else {
				if (!is_writable (dst, size))
					throw_NO_PERMISSION ();
			}
		}

		assert (dst);

		if (dst == src) { // Special case - change protection.
			// We allow to change protection of the not-allocated memory.
			// This is necessary for core services like module load.
			if (src_prot_mask & ((flags & Memory::READ_ONLY) ? PageState::MASK_RW : PageState::MASK_RO))
				space_.change_protection (src, size, flags);
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
							// Copy overlapped part with Memory::DECOMMIT.
							BYTE* first_part_end = round_up (d_end - ((BYTE*)src + size - d_end), ALLOCATION_GRANULARITY);
							assert (first_part_end < d_end);
							UWord first_part_flags = (flags & ~Memory::RELEASE) | Memory::DECOMMIT;
							while (d_p < first_part_end) {
								Block block (d_p);
								BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
								size_t cb = block_end - d_p;
								block.aligned_copy (s_p, cb, first_part_flags);
								d_p = block_end;
								s_p += cb;
							}
						}
						while (d_p < d_end) {
							Block block (d_p);
							BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
							if (block_end > d_end)
								block_end = d_end;
							size_t cb = block_end - d_p;
							block.aligned_copy (s_p, cb, flags);
							d_p = block_end;
							s_p += cb;
						}
					} else {
						BYTE* s_end = (BYTE*)src + size;
						BYTE* d_p = (BYTE*)dst + size, * s_p = (BYTE*)src + size;
						if (dst < s_end) {
							// Copy overlapped part with Memory::DECOMMIT.
							BYTE* first_part_begin = round_down ((BYTE*)dst + ((BYTE*)dst - (BYTE*)src), ALLOCATION_GRANULARITY);
							assert (first_part_begin > dst);
							UWord first_part_flags = (flags & ~Memory::RELEASE) | Memory::DECOMMIT;
							while (d_p > first_part_begin) {
								BYTE* block_begin = round_down (d_p - 1, ALLOCATION_GRANULARITY);
								Block block (block_begin);
								size_t cb = d_p - block_begin;
								s_p -= cb;
								block.aligned_copy (s_p, cb, first_part_flags);
								d_p = block_begin;
							}
						}
						while (d_p > dst) {
							BYTE* block_begin = round_down (d_p - 1, ALLOCATION_GRANULARITY);
							if (block_begin < dst)
								block_begin = (BYTE*)dst;
							Block block (block_begin);
							size_t cb = d_p - block_begin;
							s_p -= cb;
							block.aligned_copy (s_p, cb, flags);
							d_p = block_begin;
						}
					}
				} else {
					// Physical copy.
					uint32_t dst_prot_mask = commit_no_check (dst, size);
					if ((dst_prot_mask & PageState::MASK_RO) || (flags & Memory::READ_ONLY)) {
						if (dst < src) {
							BYTE* d_p = (BYTE*)dst, *d_end = d_p + size;
							BYTE* s_p = (BYTE*)src;
							while (d_p < d_end) {
								Block block (d_p);
								BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
								if (block_end > d_end)
									block_end = d_end;
								size_t cb = block_end - d_p;
								block.copy (d_p - block.address (), cb, s_p, flags);
								d_p = block_end;
								s_p += cb;
							}
						} else {
							BYTE* s_end = (BYTE*)src + size;
							BYTE* d_p = (BYTE*)dst + size, * s_p = (BYTE*)src + size;
							while (d_p > dst) {
								BYTE* block_begin = round_down (d_p - 1, ALLOCATION_GRANULARITY);
								if (block_begin < dst)
									block_begin = (BYTE*)dst;
								Block block (block_begin);
								size_t cb = d_p - block_begin;
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

			if (flags & Memory::DECOMMIT) {
				// Release or decommit source. Regions can overlap.
				Region reg = {src, size};
				if (flags & (Memory::RELEASE & ~Memory::DECOMMIT)) {
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
		if (Memory::EXACTLY & flags)
			dst = nullptr;
		else
			throw;
	}

	return dst;
}

uintptr_t ProtDomainMemory::query (const void* p, MemQuery q)
{
	{
		switch (q) {

		case MemQuery::ALLOCATION_SPACE_BEGIN:
			{
				SYSTEM_INFO sysinfo;
				GetSystemInfo (&sysinfo);
				return (uintptr_t)sysinfo.lpMinimumApplicationAddress;
			}

		case MemQuery::ALLOCATION_SPACE_END:
			return (uintptr_t)space_.end ();

		case MemQuery::ALLOCATION_UNIT:
		case MemQuery::SHARING_UNIT:
		case MemQuery::GRANULARITY:
		case MemQuery::SHARING_ASSOCIATIVITY:
		case MemQuery::OPTIMAL_COMMIT_UNIT:
			return ALLOCATION_GRANULARITY;

		case MemQuery::PROTECTION_UNIT:
		case MemQuery::COMMIT_UNIT:
			return PAGE_SIZE;

		case MemQuery::FLAGS:
			return FLAGS;
		}

		throw_BAD_PARAM ();
	}
}

bool ProtDomainMemory::is_readable (const void* p, size_t size)
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

bool ProtDomainMemory::is_private (const void* p, size_t size)
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

bool ProtDomainMemory::is_writable (const void* p, size_t size)
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

bool ProtDomainMemory::is_copy (const void* p, const void* plocal, size_t size)
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

long CALLBACK ProtDomainMemory::exception_filter (struct _EXCEPTION_POINTERS* pex)
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
				throw CORBA::MEM_NOT_COMMITTED ();
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (address, &mbi, sizeof (mbi)));
			if (MEM_MAPPED != mbi.Type) {
				Sleep (0);
				return EXCEPTION_CONTINUE_EXECUTION;
			} else if (!(mbi.Protect & PageState::MASK_ACCESS))
				throw CORBA::MEM_NOT_COMMITTED ();
			else if (pex->ExceptionRecord->ExceptionInformation [0] && !(mbi.Protect & PageState::MASK_RW))
				throw CORBA::NO_PERMISSION ();
			else
				return EXCEPTION_CONTINUE_EXECUTION;
		} else
			throw CORBA::MEM_NOT_ALLOCATED ();
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
				throw CORBA::MEM_NOT_COMMITTED ();
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (address, &mbi, sizeof (mbi)));
			if (MEM_MAPPED != mbi.Type) {
				Sleep (0);
				return;
			} else if (!(mbi.Protect & PageState::MASK_ACCESS))
				throw CORBA::MEM_NOT_COMMITTED ();
			else if (pex->ExceptionRecord->ExceptionInformation [0] && !(mbi.Protect & PageState::MASK_RW))
				throw_NO_PERMISSION ();
			else
				return;
		} else
			throw CORBA::MEM_NOT_ALLOCATED ();
	}

	throw_UNKNOWN ();
}

}
}
}
