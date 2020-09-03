#include "../Port/ProtDomainMemory.h"
#include "AddressSpace.h"
#include <BackOff.h>
#include <CORBA/CORBA.h>
#include <algorithm>

#define OBJ_NAME_PREFIX L"Nirvana"

namespace Nirvana {
namespace Core {
namespace Windows {

AddressSpace::Block::Block (AddressSpace& space, void* address) :
	space_ (space),
	address_ (round_down ((BYTE*)address, ALLOCATION_GRANULARITY)),
	info_ (*space.allocated_block (address))
{
	if (!&info_)
		throw CORBA::BAD_PARAM ();
}

const AddressSpace::Block::State& AddressSpace::Block::state ()
{
	if (State::INVALID == state_.state) {
		MEMORY_BASIC_INFORMATION mbi;
		for (/*BackOff bo*/; true; /*bo ()*/) {
			// Concurrency
			space_.query (address_, mbi);
			HANDLE hm = mapping ();
			assert (hm);
			if (!hm || INVALID_HANDLE_VALUE == hm || mbi.Type == MEM_MAPPED)
				break;
			// Memory block is in temporary state because of remapping.
		}

		DWORD page_state_bits = mbi.Protect;
		if (mbi.Type == MEM_MAPPED) {
			assert (mapping () != INVALID_HANDLE_VALUE);
			assert (mbi.AllocationBase == address_);
			state_.state = State::MAPPED;
			BYTE* page = address_;
			BYTE* block_end = page + ALLOCATION_GRANULARITY;
			auto* ps = state_.mapped.page_state;
			for (;;) {
				BYTE* end = page + mbi.RegionSize;
				assert (end <= block_end);
				page_state_bits |= mbi.Protect;
				for (; page < end; page += PAGE_SIZE) {
					*(ps++) = mbi.Protect;
				}
				if (end < block_end)
					space_.query (end, mbi);
				else
					break;
			}
		} else {
			assert (mapping () == INVALID_HANDLE_VALUE);
			assert ((BYTE*)mbi.BaseAddress + mbi.RegionSize >= (address_ + ALLOCATION_GRANULARITY));
			state_.state = mbi.Type;

			state_.reserved.begin = (BYTE*)mbi.AllocationBase;
			state_.reserved.end = (BYTE*)mbi.BaseAddress + mbi.RegionSize;
		}
		state_.page_state_bits = page_state_bits;
	}
	return state_;
}

void AddressSpace::Block::map (HANDLE mapping, MappingType protection, bool commit)
{
	assert (mapping);

	invalidate_state ();
	HANDLE old = commit ?
		InterlockedCompareExchangePointer (&info_.mapping, mapping, INVALID_HANDLE_VALUE)
		:
		InterlockedExchangePointer (&info_.mapping, mapping);

	if (old == INVALID_HANDLE_VALUE) {
		// Block is reserved.
#ifdef _DEBUG
		{
			MEMORY_BASIC_INFORMATION mbi;
			space_.query (address_, mbi);
			assert (MEM_RESERVE == mbi.State);
		}
#endif
		// If the reserved placeholder size is more than block, we must split it.
		VirtualFreeEx (space_.process (), address_, ALLOCATION_GRANULARITY, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
		// VirtualFreeEx will return TRUE if placeholder was splitted or FALSE if the placeholder is one block.
		// Both results are normal.
	} else if (old) {
		// Block is committed.
#ifdef _DEBUG
		{
			MEMORY_BASIC_INFORMATION mbi;
			space_.query (address_, mbi);
			assert (MEM_COMMIT == mbi.State);
		}
#endif
		if (commit) {
			// We don't need the new handle to commit. Close it and return.
			CloseHandle (mapping);
			return;
		}
		verify (UnmapViewOfFile2 (space_.process (), address_, MEM_PRESERVE_PLACEHOLDER));
		verify (CloseHandle (old));
	} else {
		info_.mapping = 0;
		throw CORBA::INTERNAL ();
	}

	verify (MapViewOfFile3 (mapping, space_.process (), address_, 0, ALLOCATION_GRANULARITY, MEM_REPLACE_PLACEHOLDER, protection, nullptr, 0));
}

void AddressSpace::Block::unmap (HANDLE reserve, bool no_close_handle)
{
	HANDLE mapping = InterlockedExchangePointer (&info_.mapping, reserve);
	if (!mapping) {
		if (reserve)
			info_.mapping = 0;
		throw CORBA::INTERNAL ();
	}
	if (INVALID_HANDLE_VALUE != mapping) {
		verify (UnmapViewOfFile2 (space_.process (), address_, reserve ? MEM_PRESERVE_PLACEHOLDER : 0));
		if (!no_close_handle)
			verify (CloseHandle (mapping));
	}
}

bool AddressSpace::Block::has_data_outside_of (size_t offset, size_t size, DWORD mask)
{
	size_t offset_end = offset + size;
	assert (offset_end <= ALLOCATION_GRANULARITY);
	if (offset || size < ALLOCATION_GRANULARITY) {
		auto page_state = state ().mapped.page_state;
		if (offset) {
			for (auto ps = page_state, end = page_state + (offset + PAGE_SIZE - 1) / PAGE_SIZE; ps < end; ++ps) {
				if (mask & *ps)
					return true;
			}
		}
		if (offset_end < ALLOCATION_GRANULARITY) {
			for (auto ps = page_state + (offset_end) / PAGE_SIZE, end = page_state + PAGES_PER_BLOCK; ps < end; ++ps) {
				if (mask & *ps)
					return true;
			}
		}
	}
	return false;
}

void AddressSpace::Block::copy (Block& src, size_t offset, size_t size, UWord flags)
{
	assert (size);
	size_t offset_end = offset + size;
	assert (offset_end <= ALLOCATION_GRANULARITY);
	HANDLE src_mapping = src.mapping ();
	assert (src_mapping && INVALID_HANDLE_VALUE != src_mapping);
	assert (address () != src.address ());

	bool remap;
	HANDLE cur_mapping = mapping ();
	if (INVALID_HANDLE_VALUE == cur_mapping)
		remap = true;
	else if (!CompareObjectHandles (cur_mapping, src_mapping)) {
		// Change mapping, if possible
		if (has_data_outside_of (offset, size))
			throw CORBA::INTERNAL ();
		remap = true;
	} else
		remap = false;

	copy (remap, src.can_move (offset, size, flags), src, offset, size, flags);
}

void AddressSpace::Block::copy (bool remap, bool move, Block& src, size_t offset, size_t size, UWord flags)
{
	DWORD dst_page_state [PAGES_PER_BLOCK];
	DWORD* dst_ps_begin = dst_page_state + offset / PAGE_SIZE;
	DWORD* dst_ps_end = dst_page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
	std::fill (dst_page_state, dst_ps_begin, PageState::DECOMMITTED);
	std::fill (dst_ps_end, dst_page_state + PAGES_PER_BLOCK, PageState::DECOMMITTED);

	bool no_duplicate_handle = false;
	if (move) {
		// Decide target page states based on source page states.
		auto src_page_state = src.state ().mapped.page_state;
		const DWORD* src_ps = src_page_state + offset / PAGE_SIZE;
		DWORD* dst_ps = dst_ps_begin;
		do {
			DWORD src_state = *src_ps;
			if (flags & Memory::READ_ONLY)
				if (src_state & PageState::MASK_MAY_BE_SHARED)
					*dst_ps = PageState::RO_MAPPED_SHARED;
				else
					*dst_ps = PageState::RO_MAPPED_PRIVATE;
			else
				if (src_state & PageState::MASK_MAY_BE_SHARED)
					*dst_ps = PageState::RW_MAPPED_SHARED;
				else
					*dst_ps = PageState::RW_MAPPED_PRIVATE;
			++src_ps;

		} while (++dst_ps != dst_ps_end);

		no_duplicate_handle = space_.is_current_process ();
	} else
		std::fill (dst_ps_begin, dst_ps_end, Memory::READ_ONLY & flags ? PageState::RO_MAPPED_SHARED : PageState::RW_MAPPED_SHARED);

	if (remap) {
		if (!no_duplicate_handle) {
			HANDLE mapping;
			if (!DuplicateHandle (GetCurrentProcess (), src.mapping (), space_.process (), &mapping, 0, FALSE, DUPLICATE_SAME_ACCESS))
				throw CORBA::NO_MEMORY ();
			try {
				map (mapping, move ? MAP_PRIVATE : MAP_SHARED);
			} catch (...) {
				CloseHandle (mapping);
				throw;
			}
		} else
			map (src.mapping (), MAP_PRIVATE);
	}

	if (Memory::DECOMMIT & flags) {
		if ((Memory::RELEASE & ~Memory::DECOMMIT) & flags)
			src.unmap (0, no_duplicate_handle);
		else if (move)
			src.unmap (INVALID_HANDLE_VALUE, no_duplicate_handle);
		else
			src.decommit (offset, size);
	}

	// Manage protection of copyed pages
	const DWORD* cur_ps = state ().mapped.page_state;
	const DWORD* region_begin = dst_page_state, *block_end = dst_page_state + PAGES_PER_BLOCK;
	do {
		DWORD state;
		while (!(PageState::MASK_ACCESS & (*cur_ps ^ (state = *region_begin)))) {
			++cur_ps;
			if (++region_begin == block_end)
				return;
		}
		auto region_end = region_begin;
		do {
			++cur_ps;
			++region_end;
		} while (region_end < block_end && state == *region_end);

		BYTE* ptr = address () + (region_begin - dst_page_state) * PAGE_SIZE;
		size_t size = (region_end - region_begin) * PAGE_SIZE;
		space_.protect (ptr, size, state);
		invalidate_state ();

		region_begin = region_end;
	} while (region_begin < block_end);
}

void AddressSpace::Block::change_protection (size_t offset, size_t size, UWord flags)
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

	if (flags & Memory::READ_ONLY) {
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

	auto page_state = state ().mapped.page_state;
	auto region_begin = page_state + offset / PAGE_SIZE, state_end = page_state + offset_end / PAGE_SIZE;
	do {
		auto region_end = region_begin;
		DWORD state = *region_begin;
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
				space_.protect (ptr, size, new_state);
			}
		}

		region_begin = region_end;
	} while (region_begin < state_end);
}

DWORD AddressSpace::Block::check_committed (size_t offset, size_t size)
{
	assert (offset + size <= ALLOCATION_GRANULARITY);

	const State& bs = state ();
	if (bs.state != State::MAPPED)
		throw CORBA::BAD_PARAM ();
	for (auto ps = bs.mapped.page_state + offset / PAGE_SIZE, end = bs.mapped.page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE; ps < end; ++ps)
		if (!(PageState::MASK_ACCESS & *ps))
			throw CORBA::BAD_PARAM ();
	return bs.page_state_bits;
}

void AddressSpace::Block::decommit (size_t offset, size_t size)
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
				space_.protect (address () + offset, offset_end - offset, PageState::DECOMMITTED | PAGE_REVERT_TO_FILE_MAP);

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
						verify (VirtualAlloc2 (space_.process (), ptr, size, MEM_RESET, PageState::DECOMMITTED, nullptr, 0));
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

void AddressSpace::initialize (DWORD process_id, HANDLE process_handle)
{
	process_ = process_handle;

	static const WCHAR fmt [] = OBJ_NAME_PREFIX L".mmap.%08X";
	WCHAR name [_countof (fmt) + 8 - 3];
	wsprintfW (name, fmt, process_id);

	SYSTEM_INFO si;
	GetSystemInfo (&si);
	directory_size_ = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY;

	if (GetCurrentProcessId () == process_id) {
		LARGE_INTEGER size;
		size.QuadPart = directory_size_ * sizeof (BlockInfo);
		mapping_ = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, name);
		if (!mapping_)
			throw CORBA::INITIALIZE ();
	} else {
		mapping_ = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, name);
		if (!mapping_)
			throw CORBA::INITIALIZE ();
	}

#ifdef _WIN64
	directory_ = (BlockInfo**)VirtualAlloc (0, (directory_size_ + SECOND_LEVEL_BLOCK - 1) / SECOND_LEVEL_BLOCK * sizeof (BlockInfo*), MEM_RESERVE, PAGE_READWRITE);
#else
	directory_ = (BlockInfo*)MapViewOfFile (mapping_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#endif
	if (!directory_)
		throw CORBA::INITIALIZE ();
}

void AddressSpace::terminate ()
{
	if (directory_) {
#ifdef _WIN64
		BlockInfo** end = directory_ + (directory_size_ + SECOND_LEVEL_BLOCK - 1) / SECOND_LEVEL_BLOCK;
		for (BlockInfo** page = directory_; page < end; page += PAGE_SIZE / sizeof (BlockInfo**)) {
			MEMORY_BASIC_INFORMATION mbi;
			verify (VirtualQuery (page, &mbi, sizeof (mbi)));
			if (mbi.State == MEM_COMMIT) {
				BlockInfo** end = page + PAGE_SIZE / sizeof (BlockInfo**);
				for (BlockInfo** p = page; p < end; ++p) {
					BlockInfo* block = *p;
					if (block) {
#ifdef _DEBUG
						if (GetCurrentProcess () == process_) {
							BYTE* address = (BYTE*)((p - directory_) * SECOND_LEVEL_BLOCK * ALLOCATION_GRANULARITY);
							for (BlockInfo* page = block, *end = block + SECOND_LEVEL_BLOCK; page != end; page += PAGE_SIZE / sizeof (BlockInfo)) {
								verify (VirtualQuery (page, &mbi, sizeof (mbi)));
								if (mbi.State == MEM_COMMIT) {
									for (BlockInfo* p = page, *end = page + PAGE_SIZE / sizeof (BlockInfo); p != end; ++p, address += ALLOCATION_GRANULARITY) {
										HANDLE hm = p->mapping;
										if (INVALID_HANDLE_VALUE == hm) {
											VirtualFreeEx (process_, address, 0, MEM_RELEASE);
										} else {
											if (hm) {
												UnmapViewOfFile2 (process_, address, 0);
												CloseHandle (hm);
											}
										}
									}
								} else
									address += PAGE_SIZE / sizeof (BlockInfo) * ALLOCATION_GRANULARITY;
							}
						}
#endif
						verify (UnmapViewOfFile (block));
					}
				}
			}
		}
		verify (VirtualFree (directory_, 0, MEM_RELEASE));
#else
#ifdef _DEBUG
		if (GetCurrentProcess () == process_) {
			BYTE* address = 0;
			for (BlockInfo* page = directory_, *end = directory_ + directory_size_; page < end; page += PAGE_SIZE / sizeof (BlockInfo)) {
				MEMORY_BASIC_INFORMATION mbi;
				verify (VirtualQuery (page, &mbi, sizeof (mbi)));
				if (mbi.State == MEM_COMMIT) {
					for (BlockInfo* p = page, *end = page + PAGE_SIZE / sizeof (BlockInfo); p != end; ++p, address += ALLOCATION_GRANULARITY) {
						HANDLE hm = p->mapping;
						if (INVALID_HANDLE_VALUE == hm) {
							VirtualFreeEx (process_, address, 0, MEM_RELEASE);
						} else {
							if (hm) {
								UnmapViewOfFile2 (process_, address, 0);
								CloseHandle (hm);
							}
						}
					}
				} else
					address += PAGE_SIZE / sizeof (BlockInfo) * ALLOCATION_GRANULARITY;
			}
		}
#endif
		verify (UnmapViewOfFile (directory_));
#endif
		directory_ = 0;
	}
	if (mapping_) {
		verify (CloseHandle (mapping_));
		mapping_ = 0;
	}
}

AddressSpace::BlockInfo& AddressSpace::block (const void* address)
{
	size_t idx = (size_t)address / ALLOCATION_GRANULARITY;
	assert (idx < directory_size_);
	BlockInfo* p;
#ifdef _WIN64
	size_t i0 = idx / SECOND_LEVEL_BLOCK;
	size_t i1 = idx % SECOND_LEVEL_BLOCK;
	if (!VirtualAlloc (directory_ + i0, sizeof (BlockInfo*), MEM_COMMIT, PAGE_READWRITE))
		throw NO_MEMORY ();
	BlockInfo** pp = directory_ + i0;
	p = *pp;
	if (!p) {
		LARGE_INTEGER offset;
		offset.QuadPart = ALLOCATION_GRANULARITY * i0;
		p = (BlockInfo*)MapViewOfFile (mapping_, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, ALLOCATION_GRANULARITY);
		if (!p)
			throw NO_MEMORY ();
		BlockInfo* cur = (BlockInfo*)InterlockedCompareExchangePointer ((void* volatile*)pp, p, 0);
		if (cur) {
			UnmapViewOfFile (p);
			p = cur;
		}
	}
	p += i1;
#else
	p = directory_ + idx;
#endif
	if (!VirtualAlloc (p, sizeof (BlockInfo), MEM_COMMIT, PAGE_READWRITE))
		throw CORBA::NO_MEMORY ();
	return *p;
}

AddressSpace::BlockInfo* AddressSpace::allocated_block (const void* address)
{
	size_t idx = (size_t)address / ALLOCATION_GRANULARITY;
	BlockInfo* p = 0;
	if (idx < directory_size_) {
		MEMORY_BASIC_INFORMATION mbi;
#ifdef _WIN64
		size_t i0 = idx / SECOND_LEVEL_BLOCK;
		size_t i1 = idx % SECOND_LEVEL_BLOCK;
		BlockInfo** pp = directory_ + i0;
		verify (VirtualQuery (pp, &mbi, sizeof (mbi)));
		if (mbi.State == MEM_COMMIT) {
			if (p = *pp)
				p += i1;
		}
#else
		p = directory_ + idx;
		verify (VirtualQuery (p, &mbi, sizeof (mbi)));
		if (mbi.State != MEM_COMMIT)
			p = 0;
#endif
		if (p && !p->mapping)
			p = 0;
	}
	return p;
}

void* AddressSpace::reserve (size_t size, UWord flags, void* dst)
{
	if (!size)
		throw CORBA::BAD_PARAM ();

	BYTE* p;
	BYTE* tgt;
	if (dst) {
		tgt = round_down ((BYTE*)dst, ALLOCATION_GRANULARITY);
		if (flags & Memory::EXACTLY)
			size = round_up ((BYTE*)dst + size - tgt, ALLOCATION_GRANULARITY);
		else
			size = round_up (size, ALLOCATION_GRANULARITY);
	} else {
		tgt = nullptr;
		size = round_up (size, ALLOCATION_GRANULARITY);
	}
	p = (BYTE*)VirtualAlloc2 (process_, dst, size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
	if (!p) {
		if (flags & Memory::EXACTLY)
			return nullptr;
		else
			throw CORBA::NO_MEMORY ();
	}

	{
		BYTE* pb = p;
		try {
			for (BYTE* end = p + size; pb < end; pb += ALLOCATION_GRANULARITY) {
				assert (block (pb).mapping == nullptr);
				block (pb).mapping = INVALID_HANDLE_VALUE;
			}
		} catch (...) {
			while (pb > p) {
				pb -= ALLOCATION_GRANULARITY;
				block (pb).mapping = nullptr;
			}
			VirtualFreeEx (process_, p, 0, MEM_RELEASE);
			throw;
		}
	}

	if (dst && (flags & Memory::EXACTLY))
		return dst;
	else
		return p;
}

void AddressSpace::release (void* dst, size_t size)
{
	if (!(dst && size))
		return;

	BYTE* begin = round_down ((BYTE*)dst, ALLOCATION_GRANULARITY);
	BYTE* end = round_up ((BYTE*)dst + size, ALLOCATION_GRANULARITY);

	// Check allocation.
	for (BYTE* p = begin; p != end; p += ALLOCATION_GRANULARITY) {
		if (!allocated_block (p))
			throw CORBA::BAD_PARAM ();
	}

	{ // Define allocation margins if memory is reserved.
		MEMORY_BASIC_INFORMATION begin_mbi = {0}, end_mbi = {0};
		if (INVALID_HANDLE_VALUE == allocated_block (begin)->mapping) {
			query (begin, begin_mbi);
			assert (MEM_RESERVE == begin_mbi.State);
			if ((BYTE*)begin_mbi.BaseAddress + begin_mbi.RegionSize >= end)
				end_mbi = begin_mbi;
		}

		if (!end_mbi.BaseAddress) {
			BYTE* back = end - PAGE_SIZE;
			if (INVALID_HANDLE_VALUE == allocated_block (back)->mapping) {
				query (back, end_mbi);
				assert (MEM_RESERVE == end_mbi.State);
			}
		}

		// Split reserved blocks at begin and end if need.
		if (begin_mbi.BaseAddress) {
			SSIZE_T realloc = begin - (BYTE*)begin_mbi.AllocationBase;
			if (realloc > 0)
				verify (VirtualFreeEx (process_, begin_mbi.AllocationBase, realloc, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER));
		}

		if (end_mbi.BaseAddress) {
			SSIZE_T realloc = (BYTE*)end_mbi.BaseAddress + end_mbi.RegionSize - end;
			if (realloc > 0)
				verify (VirtualFreeEx (process_, end, realloc, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER));
		}
	}

	// Release memory
	for (BYTE* p = begin; p < end;) {
		HANDLE mapping = InterlockedExchangePointer (&allocated_block (p)->mapping, 0);
		assert (mapping);
		if (INVALID_HANDLE_VALUE == mapping) {
			MEMORY_BASIC_INFORMATION mbi;
			query (p, mbi);
			assert (mbi.State == MEM_RESERVE);
			verify (VirtualFreeEx (process_, p, 0, MEM_RELEASE));
			BYTE* region_end = (BYTE*)mbi.BaseAddress + mbi.RegionSize;
			if (region_end > end)
				region_end = end;
			p += ALLOCATION_GRANULARITY;
			while (p < region_end) {
				assert (INVALID_HANDLE_VALUE == block (p).mapping);
				allocated_block (p)->mapping = 0;
				p += ALLOCATION_GRANULARITY;
			}
		} else if (mapping) {
			verify (UnmapViewOfFile2 (process_, p, 0));
			verify (CloseHandle (mapping));
			p += ALLOCATION_GRANULARITY;
		}
	}
}

void AddressSpace::check_allocated (void* ptr, size_t size)
{
	if (!size)
		return;
	if (!ptr)
		throw CORBA::BAD_PARAM ();

	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end; p += ALLOCATION_GRANULARITY)
		if (!allocated_block (p))
			throw CORBA::BAD_PARAM ();
}

DWORD AddressSpace::check_committed (void* ptr, size_t size)
{
	if (!size)
		return 0;
	if (!ptr)
		throw CORBA::BAD_PARAM ();

	DWORD mask;
	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end;) {
		Block block (*this, p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		mask |= block.check_committed (p - block.address (), block_end - p);
		p = block_end;
	}
	return mask;
}

void AddressSpace::change_protection (void* ptr, size_t size, UWord flags)
{
	if (!size)
		return;
	if (!ptr)
		throw CORBA::BAD_PARAM ();

	BYTE* begin = (BYTE*)ptr;
	BYTE* end = begin + size;
	for (BYTE* p = begin; p < end;) {
		Block block (*this, p);
		BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
		if (block_end > end)
			block_end = end;
		block.change_protection (p - block.address (), block_end - p, flags);
		p = block_end;
	}
}

}
}
}
