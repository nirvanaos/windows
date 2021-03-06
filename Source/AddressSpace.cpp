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
#include "../Port/Memory.h"
#include "AddressSpace.h"
#include <algorithm>
#include <winternl.h>

// wsprintfW
#pragma comment (lib, "User32.lib")

namespace Nirvana {
namespace Core {
namespace Windows {

ULONG handle_count (HANDLE h)
{
	PUBLIC_OBJECT_BASIC_INFORMATION info;
	if (!NtQueryObject (h, ObjectBasicInformation, &info, sizeof (info), nullptr))
		return info.HandleCount;
	assert (false);
	return 0;
}

void BlockState::query (HANDLE process)
{
	verify (QueryWorkingSetEx (process, page_state, sizeof (page_state)));
	PageState* ps = page_state;
	do {
		assert (ps->VirtualAttributes.Valid || 0 == ps->VirtualAttributes.Win32Protection);
		// If committed page was not accessed, it's state remains invalid.
		// For such pages we call VirtualQuery to obtain memory protection.
		if (!ps->VirtualAttributes.Valid && ps->VirtualAttributes.Shared) {
			MEMORY_BASIC_INFORMATION mbi;
			VirtualQueryEx (process, ps->VirtualAddress, &mbi, sizeof (mbi));
			BYTE* end = (BYTE*)mbi.BaseAddress + mbi.RegionSize;
			do {
				ps->VirtualAttributes.Win32Protection = mbi.Protect;
				++ps;
			} while (ps < std::end (page_state) && ps->VirtualAddress < end);
		} else
			++ps;
	} while (ps < std::end (page_state));
}

AddressSpace::Block::Block (AddressSpace& space, void* address, bool exclusive) :
	space_ (space),
	info_ (check_block (space.allocated_block (address))),
	block_state_ (round_down ((BYTE*)address, ALLOCATION_GRANULARITY)),
	exclusive_ (exclusive)
{
	mapping_ = exclusive ? info_.mapping.exclusive_lock () : info_.mapping.lock ();
	if (!mapping_) {
		if (exclusive)
			info_.mapping.set_and_unlock (nullptr);
		else
			info_.mapping.unlock ();
		throw_BAD_PARAM ();
	}
	if (INVALID_HANDLE_VALUE == mapping_)
		state_ = State::RESERVED;
	else
		state_ = State::PAGE_STATE_UNKNOWN;
}

AddressSpace::Block::~Block ()
{
	if (exclusive_)
		info_.mapping.set_and_unlock (mapping_);
	else
		info_.mapping.unlock ();
}

bool AddressSpace::Block::exclusive_lock ()
{
	if (!exclusive_) {
		info_.mapping.unlock ();
		mapping_ = info_.mapping.exclusive_lock ();
		exclusive_ = true;
		if (!mapping_)
			throw_INTERNAL (); // The block was unexpectedly released in another thread.
		invalidate_state ();
		return true;
	}
	return false;
}

const BlockState& AddressSpace::Block::state ()
{
	if (State::PAGE_STATE_UNKNOWN == state_) {
		block_state_.query (space_.process ());
		state_ = State::MAPPED;
	}
	return block_state_;
}

void AddressSpace::Block::map (HANDLE hm, DWORD protection)
{
	assert (hm);
	assert (exclusive_);

	HANDLE old = mapping ();
	assert (old);
	mapping (hm);

	invalidate_state ();
	if (old == INVALID_HANDLE_VALUE) {
		// Block is reserved.
#ifdef _DEBUG
		{
			MEMORY_BASIC_INFORMATION mbi;
			space_.query (address (), mbi);
			assert (MEM_RESERVE == mbi.State);
		}
#endif
		// If the reserved placeholder size is more than block, we must split it.
		VirtualFreeEx (space_.process (), address (), ALLOCATION_GRANULARITY, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
		// VirtualFreeEx will return TRUE if placeholder was splitted or FALSE if the placeholder is one block.
		// Both results are normal.
	} else {
		// Block is committed.
#ifdef _DEBUG
		{
			MEMORY_BASIC_INFORMATION mbi;
			space_.query (address (), mbi);
			assert (MEM_COMMIT == mbi.State);
		}
#endif
		verify (UnmapViewOfFile2 (space_.process (), address (), MEM_PRESERVE_PLACEHOLDER));
		verify (CloseHandle (old));
	}

	verify (MapViewOfFile3 (hm, space_.process (), address (), 0, ALLOCATION_GRANULARITY, MEM_REPLACE_PLACEHOLDER, protection, nullptr, 0));
}

void AddressSpace::Block::unmap ()
{
	if (!exclusive_) {
		if (INVALID_HANDLE_VALUE == mapping ())
			return;
		exclusive_lock ();
	}
	HANDLE hm = mapping ();
	assert (hm);
	if (INVALID_HANDLE_VALUE != hm) {
		verify (UnmapViewOfFile2 (space_.process (), address (), MEM_PRESERVE_PLACEHOLDER));
		verify (CloseHandle (hm));
		mapping (INVALID_HANDLE_VALUE);
	}
}

bool AddressSpace::Block::has_data (size_t offset, size_t size, DWORD mask)
{
	size_t offset_end = offset + size;
	assert (offset_end <= ALLOCATION_GRANULARITY);
	auto page_state = state ().page_state;
	for (auto ps = page_state + (offset + PAGE_SIZE - 1) / PAGE_SIZE, end = page_state + offset_end / PAGE_SIZE; ps < end; ++ps) {
		if (mask & ps->state ())
			return true;
	}
	return false;
}

bool AddressSpace::Block::has_data_outside_of (size_t offset, size_t size, DWORD mask)
{
	size_t offset_end = offset + size;
	assert (offset_end <= ALLOCATION_GRANULARITY);
	if (offset || size < ALLOCATION_GRANULARITY) {
		auto page_state = state ().page_state;
		if (offset) {
			for (auto ps = page_state, end = page_state + (offset + PAGE_SIZE - 1) / PAGE_SIZE; ps < end; ++ps) {
				if (mask & ps->state ())
					return true;
			}
		}
		if (offset_end < ALLOCATION_GRANULARITY) {
			for (auto ps = page_state + offset_end / PAGE_SIZE, end = page_state + PAGES_PER_BLOCK; ps < end; ++ps) {
				if (mask & ps->state ())
					return true;
			}
		}
	}
	return false;
}

void AddressSpace::Block::copy (Block& src, size_t offset, size_t size, unsigned flags)
{
	exclusive_lock ();
	assert (size);
	assert (offset + size <= ALLOCATION_GRANULARITY);

restart:
	HANDLE src_mapping = src.mapping ();
	assert (src_mapping && INVALID_HANDLE_VALUE != src_mapping);
	assert (address () != src.address ());

	bool remap;
	HANDLE cur_mapping = mapping ();
	if (INVALID_HANDLE_VALUE == cur_mapping)
		remap = true;
	else {
		if (!CompareObjectHandles (cur_mapping, src_mapping)) {
			// Change mapping
			assert (!has_data_outside_of (offset, size));
			remap = true;
		} else
			remap = false;
	}

	bool move = src.can_move (offset, size, flags);

	if (move && src.exclusive_lock ())
		goto restart;

	DWORD dst_page_state [PAGES_PER_BLOCK];
	DWORD* dst_ps_begin = dst_page_state + offset / PAGE_SIZE;
	DWORD* dst_ps_end = dst_page_state + (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
	std::fill (dst_page_state, dst_ps_begin, PageState::DECOMMITTED);
	std::fill (dst_ps_end, dst_page_state + PAGES_PER_BLOCK, PageState::DECOMMITTED);

	DWORD copied_pages_state;
	if (Memory::READ_ONLY & flags)
		copied_pages_state = PageState::READ_ONLY;
	else if (!move || handle_count (src.mapping ()) > 1)
		copied_pages_state = PageState::READ_WRITE_SHARED;
	else
		copied_pages_state = PageState::READ_WRITE_PRIVATE;
	std::fill (dst_ps_begin, dst_ps_end, copied_pages_state);

	if (remap) {
		HANDLE hm;
		if (!DuplicateHandle (GetCurrentProcess (), src.mapping (), space_.process (), &hm, 0, FALSE, DUPLICATE_SAME_ACCESS))
			throw_NO_MEMORY ();
		try {
			map (hm, copied_pages_state);
		} catch (...) {
			CloseHandle (hm);
			throw;
		}
	}

	// Manage protection of copied pages
	const PageState* cur_ps = state ().page_state;
	const DWORD* region_begin = dst_page_state, *block_end = dst_page_state + PAGES_PER_BLOCK;
	do {
		DWORD protection;
		while (!(PageState::MASK_ACCESS & (cur_ps->protection () ^ (protection = *region_begin)))) {
			// We need to change access of the copied pages
			++cur_ps;
			if (++region_begin == block_end)
				return;
		}
		auto region_end = region_begin;
		do {
			++cur_ps;
			++region_end;
		} while (region_end < block_end && protection == *region_end);

		BYTE* ptr = address () + (region_begin - dst_page_state) * PAGE_SIZE;
		size_t size = (region_end - region_begin) * PAGE_SIZE;
		space_.protect (ptr, size, protection);
		invalidate_state ();

		region_begin = region_end;
	} while (region_begin < block_end);
}

AddressSpace::AddressSpace (DWORD process_id, HANDLE process_handle) :
	process_ (process_handle),
	mapping_ (nullptr),
	directory_ (nullptr)
{
	assert (!mapping_ && !directory_);

	static const WCHAR fmt [] = OBJ_NAME_PREFIX WINWCS (".mmap.%08X");
	WCHAR name [_countof (fmt) + 8 - 3];
	wsprintfW (name, fmt, process_id);

	SYSTEM_INFO si;
	GetSystemInfo (&si);
	directory_size_ = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY;

	if (GetCurrentProcessId () == process_id) {
		LARGE_INTEGER size;
		size.QuadPart = (LONGLONG)(directory_size_ * sizeof (BlockInfo));
		mapping_ = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, name);
		if (!mapping_)
			throw_INITIALIZE ();
	} else {
		mapping_ = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, name);
		if (!mapping_)
			throw_INITIALIZE ();
	}

#ifdef _WIN64
	directory_ = (BlockInfo**)VirtualAlloc (0, (directory_size_ + SECOND_LEVEL_BLOCK - 1) / SECOND_LEVEL_BLOCK * sizeof (BlockInfo*), MEM_RESERVE, PAGE_READWRITE);
#else
	directory_ = (BlockInfo*)MapViewOfFile (mapping_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
#endif
	if (!directory_) {
		verify (CloseHandle (mapping_));
		mapping_ = nullptr;
		throw_INITIALIZE ();
	}
}

AddressSpace::~AddressSpace () NIRVANA_NOEXCEPT
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
										HANDLE hm = p->mapping.handle ();
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
						HANDLE hm = p->mapping.handle ();
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
		directory_ = nullptr;
	}
	if (mapping_) {
		verify (CloseHandle (mapping_));
		mapping_ = nullptr;
	}
}

AddressSpace::BlockInfo* AddressSpace::block_no_commit (const void* address)
{
	size_t idx = (size_t)address / ALLOCATION_GRANULARITY;
	assert (idx < directory_size_);
	BlockInfo* p;
#ifdef _WIN64
	size_t i0 = idx / SECOND_LEVEL_BLOCK;
	size_t i1 = idx % SECOND_LEVEL_BLOCK;
	if (!VirtualAlloc (directory_ + i0, sizeof (BlockInfo*), MEM_COMMIT, PAGE_READWRITE))
		throw_NO_MEMORY ();
	BlockInfo** pp = directory_ + i0;
	p = *pp;
	if (!p) {
		LARGE_INTEGER offset;
		offset.QuadPart = ALLOCATION_GRANULARITY * i0;
		p = (BlockInfo*)MapViewOfFile (mapping_, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, ALLOCATION_GRANULARITY);
		if (!p)
			throw_NO_MEMORY ();
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
	return p;
}

AddressSpace::BlockInfo& AddressSpace::block (const void* address)
{
	BlockInfo* p = block_no_commit (address);
	if (!VirtualAlloc (p, sizeof (BlockInfo), MEM_COMMIT, PAGE_READWRITE))
		throw_NO_MEMORY ();
	return *p;
}

AddressSpace::BlockInfo* AddressSpace::allocated_block (const void* address)
{
	BlockInfo* p = nullptr;
	size_t idx = (size_t)address / ALLOCATION_GRANULARITY;
	if (idx < directory_size_) {
		p = block_no_commit (address);
		MEMORY_BASIC_INFORMATION mbi;
		verify (VirtualQuery (p, &mbi, sizeof (mbi)));
		if (mbi.State != MEM_COMMIT || !p->mapping)
			p = nullptr;
	}
	return p;
}

void* AddressSpace::reserve (void* dst, size_t& size, unsigned flags)
{
	if (!size)
		throw_BAD_PARAM ();

	BYTE* p;
	BYTE* tgt;
	if (dst) {
		tgt = round_down ((BYTE*)dst, ALLOCATION_GRANULARITY);
		if (flags & Memory::EXACTLY)
			size = round_up ((size_t)((BYTE*)dst + size - tgt), ALLOCATION_GRANULARITY);
		else
			size = round_up (size, ALLOCATION_GRANULARITY);
	} else {
		tgt = nullptr;
		size = round_up (size, ALLOCATION_GRANULARITY);
	}
	p = (BYTE*)VirtualAlloc2 (process_, tgt, size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
	if (!p) {
		if (flags & Memory::EXACTLY)
			return nullptr;
		else
			throw_NO_MEMORY ();
	}

	{
		BYTE* pb = p;
		try {
			for (BYTE* end = p + size; pb < end; pb += ALLOCATION_GRANULARITY) {
				BlockInfo& bi = block (pb);
				bi.mapping.exclusive_lock ();
				assert (!bi.mapping);
				bi.mapping.init_invalid ();
			}
		} catch (...) { // NO_MEMORY for directory allocation
			while (pb > p) {
				pb -= ALLOCATION_GRANULARITY;
				block (pb).mapping.reset_on_failure ();
			}
			VirtualFreeEx (process_, p, 0, MEM_RELEASE);
			if (flags & Memory::EXACTLY)
				return nullptr;
			throw;
		}
	}

	return p;
}

void AddressSpace::release (void* dst, size_t size)
{
	if (!(dst && size))
		return;

	BYTE* const begin = round_down ((BYTE*)dst, ALLOCATION_GRANULARITY);
	BYTE* const end = round_up ((BYTE*)dst + size, ALLOCATION_GRANULARITY);

	// Check allocation and exclusive lock all released blocks.
	HANDLE begin_handle, end_handle;
	{
		BlockInfo* block = allocated_block (begin);
		if (!block)
			throw_FREE_MEM ();
		begin_handle = end_handle = block->mapping.exclusive_lock ();
		for (BYTE* p = begin + ALLOCATION_GRANULARITY; p != end; p += ALLOCATION_GRANULARITY) {
			BlockInfo* block = allocated_block (p);
			if (!block) {
				while (p > begin) {
					p -= ALLOCATION_GRANULARITY;
					allocated_block (p)->mapping.exclusive_unlock ();
				}
				throw_FREE_MEM ();
			}
			end_handle = block->mapping.exclusive_lock ();
		}
	}

	{ // Define allocation margins if memory is reserved.
		MEMORY_BASIC_INFORMATION begin_mbi = {0}, end_mbi = {0};
		if (INVALID_HANDLE_VALUE == begin_handle) {
			query (begin, begin_mbi);
			assert (MEM_RESERVE == begin_mbi.State);
			if ((BYTE*)begin_mbi.BaseAddress + begin_mbi.RegionSize >= end)
				end_mbi = begin_mbi;
		}

		if (!end_mbi.BaseAddress) {
			BYTE* back = end - PAGE_SIZE;
			if (INVALID_HANDLE_VALUE == end_handle) {
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
		BlockInfo* block = allocated_block (p);
		HANDLE mapping = block->mapping.reset_and_unlock ();
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
				mapping = allocated_block (p)->mapping.reset_and_unlock ();
				assert (INVALID_HANDLE_VALUE == mapping);
				p += ALLOCATION_GRANULARITY;
			}
		} else {
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
		throw_BAD_PARAM ();

	for (BYTE* p = (BYTE*)ptr, *end = p + size; p < end; p += ALLOCATION_GRANULARITY)
		if (!allocated_block (p))
			throw_BAD_PARAM ();
}

}
}
}
