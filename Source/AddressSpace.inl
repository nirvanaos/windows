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
#ifndef NIRVANA_CORE_WINDOWS_ADDRESSSPACE_INL_
#define NIRVANA_CORE_WINDOWS_ADDRESSSPACE_INL_
#pragma once

#include "Memory.inl"

#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
#include "rewolf-wow64ext/src/wow64ext.h"
extern DWORD64 getNTDLL64 ();

#endif

namespace Nirvana {
namespace Core {
namespace Windows {

#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
extern DWORD64 wow64_NtQueryVirtualMemory;
extern DWORD64 wow64_NtProtectVirtualMemory;
extern DWORD64 wow64_NtAllocateVirtualMemoryEx;
extern DWORD64 wow64_NtFreeVirtualMemory;
extern DWORD64 wow64_NtMapViewOfSectionEx;
extern DWORD64 wow64_NtUnmapViewOfSectionEx;
#endif

inline void address_space_init ()
{
	local_address_space.construct (GetCurrentProcessId (), GetCurrentProcess ());
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	DWORD64 ntdll = getNTDLL64 ();
	wow64_NtQueryVirtualMemory = GetProcAddress64 (ntdll, "NtQueryVirtualMemory");
	wow64_NtProtectVirtualMemory = GetProcAddress64 (ntdll, "NtProtectVirtualMemory");
	wow64_NtAllocateVirtualMemoryEx = GetProcAddress64 (ntdll, "NtAllocateVirtualMemoryEx");
	wow64_NtFreeVirtualMemory = GetProcAddress64 (ntdll, "NtFreeVirtualMemory");
	wow64_NtMapViewOfSectionEx = GetProcAddress64 (ntdll, "NtMapViewOfSectionEx");
	wow64_NtUnmapViewOfSectionEx = GetProcAddress64 (ntdll, "NtUnmapViewOfSectionEx");
#endif
}

inline void address_space_term () NIRVANA_NOEXCEPT
{
	local_address_space.destruct ();
}

template <bool x64> inline
void AddressSpace <x64>::query (Address address, MBI& mbi) const
{
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	if (x64) {
		DWORD64 status = X64Call (wow64_NtQueryVirtualMemory, 6, HANDLE_TO_DWORD64 (process_), address,
			PTR_TO_DWORD64 (&mbi), (DWORD64)sizeof (mbi), (DWORD64)0);
		assert (!status);
	} else
#endif
		verify (VirtualQueryEx (process_, (void*)(uintptr_t)address, (MEMORY_BASIC_INFORMATION*)&mbi,
			sizeof (mbi)));
}

template <bool x64> inline
void AddressSpace <x64>::protect (Address address, size_t size, uint32_t protection) const
{
	assert (!(protection & ~PageState::MASK_PROTECTION));
	assert (size && 0 == size % PAGE_SIZE);
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	if (x64) {
		DWORD64 tmp_size = size;
		DWORD64 old;
		DWORD64 status = X64Call (wow64_NtProtectVirtualMemory, 5, HANDLE_TO_DWORD64 (process_),
			PTR_TO_DWORD64 (&address), PTR_TO_DWORD64 (&tmp_size), (DWORD64)protection, PTR_TO_DWORD64 (&old));
	} else
#endif
	{
		unsigned long old;
		verify (VirtualProtectEx (process_, (void*)(uintptr_t)address, size, protection, &old));
	}
}

template <bool x64> inline
typename AddressSpace <x64>::Address AddressSpace <x64>::alloc (Address address, size_t size,
	uint32_t flags, uint32_t protection) const
{
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	if (x64) {
		DWORD64 tmp_addr = (uintptr_t)address;
		DWORD64 tmp_size = size;
		DWORD64 status = X64Call (wow64_NtAllocateVirtualMemoryEx, 6, PTR_TO_DWORD64 (&tmp_addr),
			(DWORD64)0, PTR_TO_DWORD64 (&tmp_size), (DWORD64)flags, (DWORD64)protection);
		if (!status)
			return (Address)tmp_addr;
		else
			return 0;
	} else
#endif
		return (Address)(uintptr_t)VirtualAlloc2 (process_, (void*)(uintptr_t)address, size, flags,
			protection, nullptr, 0);
}

template <bool x64> inline
bool AddressSpace <x64>::free (Address address, Size size, uint32_t flags) const
{
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	if (x64) {
		DWORD64 status = X64Call (wow64_NtFreeVirtualMemory, 5, HANDLE_TO_DWORD64 (process_),
			(DWORD64)address, (DWORD64)size, (DWORD64)flags);
		return !status;
	} else
#endif
		return VirtualFreeEx (process_, (void*)(uintptr_t)address, (size_t)size, flags);
}

template <bool x64> inline
typename AddressSpace <x64>::Address AddressSpace <x64>::map (HANDLE hm, Address address,
	size_t size, uint32_t flags, uint32_t protection) const
{
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	if (x64) {
		DWORD64 tmp_addr = (uintptr_t)address;
		DWORD64 tmp_size = size;
		DWORD64 status = X64Call (wow64_NtMapViewOfSectionEx, 9, HANDLE_TO_DWORD64 (hm),
			HANDLE_TO_DWORD64 (process_), PTR_TO_DWORD64 (&tmp_addr), (DWORD64)0, PTR_TO_DWORD64 (&tmp_size),
			(DWORD64)flags, (DWORD64)protection, (DWORD64)0, (DWORD64)0);
		if (!status)
			return (Address)tmp_addr;
		else
			return 0;
	} else
#endif
		return (Address)(uintptr_t)MapViewOfFile3 (hm, process_, (void*)(uintptr_t)address, 0, size,
			flags, protection, nullptr, 0);
}

template <bool x64> inline
bool AddressSpace <x64>::unmap (Address address, uint32_t flags) const
{
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	if (x64) {
		DWORD64 status = X64Call (wow64_NtUnmapViewOfSectionEx, 3, HANDLE_TO_DWORD64 (process_), (DWORD64)address, (DWORD64)flags);
		return !status;
	} else
#endif
		return UnmapViewOfFile2 (process_, (void*)(uintptr_t)address, flags);
}

template <bool x64>
AddressSpace <x64>::Block::Block (AddressSpace <x64>& space, Address address, bool exclusive) :
	space_ (space),
	address_ (Nirvana::round_down (address, (Size)ALLOCATION_GRANULARITY)),
	info_ (check_block (space.allocated_block (address_))),
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

template <bool x64>
AddressSpace <x64>::Block::~Block ()
{
	if (exclusive_)
		info_.mapping.set_and_unlock (mapping_);
	else
		info_.mapping.unlock ();
}

template <bool x64>
bool AddressSpace <x64>::Block::exclusive_lock ()
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

template <bool x64>
void AddressSpace <x64>::Block::map (HANDLE hm, uint32_t protection)
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
			MBI mbi;
			space_.query (address (), mbi);
			assert (MEM_RESERVE == mbi.State);
		}
#endif
		// If the reserved placeholder size is more than block, we must split it.
		space_.free (address (), ALLOCATION_GRANULARITY, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
		// VirtualFreeEx will return TRUE if placeholder was splitted or FALSE if the placeholder is one block.
		// Both results are normal.
	} else {
		// Block is committed.
#ifdef _DEBUG
		{
			MBI mbi;
			space_.query (address (), mbi);
			assert (MEM_COMMIT == mbi.State);
		}
#endif
		verify (space_.unmap (address (), MEM_PRESERVE_PLACEHOLDER));
		verify (CloseHandle (old));
	}

	verify (space_.map (hm, address (), ALLOCATION_GRANULARITY, MEM_REPLACE_PLACEHOLDER, protection));
}

template <bool x64>
void AddressSpace <x64>::Block::unmap ()
{
	if (!exclusive_) {
		if (INVALID_HANDLE_VALUE == mapping ())
			return;
		exclusive_lock ();
	}
	HANDLE hm = mapping ();
	assert (hm);
	if (INVALID_HANDLE_VALUE != hm) {
		verify (space_.unmap (address (), MEM_PRESERVE_PLACEHOLDER));
		verify (CloseHandle (hm));
		mapping (INVALID_HANDLE_VALUE);
	}
}

template <bool x64>
void AddressSpace <x64>::Block::copy (Port::Memory::Block& src, size_t offset, size_t size, uint32_t copied_pages_state)
{
	assert (exclusive_locked ());
	assert (size);
	assert (offset + size <= ALLOCATION_GRANULARITY);

	HANDLE hm;
	if (!DuplicateHandle (GetCurrentProcess (), src.mapping (), space_.process (), &hm, 0, FALSE, DUPLICATE_SAME_ACCESS))
		throw_NO_MEMORY ();
	try {
		map (hm, copied_pages_state);
	} catch (...) {
		CloseHandle (hm);
		throw;
	}

	// Disable access to the committed pages out of range.
	size_t start_page = offset / PAGE_SIZE;
	if (start_page) {
		const PageState* page_state = src.state ().page_state;
		const PageState* region_begin = page_state;
		const PageState* end = page_state + start_page;
		do {
			do {
				if (region_begin->VirtualAttributes.Win32Protection)
					break;
			} while (end > ++region_begin);
			if (region_begin < end) {
				const PageState* region_end = region_begin + 1;
				while (region_end < end) {
					if (!region_end->VirtualAttributes.Win32Protection)
						break;
					++region_end;
				}
				space_.protect ((Address)(address () + (region_begin - page_state) * PAGE_SIZE),
					(region_end - region_begin) * PAGE_SIZE, PAGE_NOACCESS);
				region_begin = region_end;
			}
		} while (region_begin < end);
	}
	size_t end_page = (offset + size + PAGE_SIZE - 1) / PAGE_SIZE;
	if (end_page < PAGES_PER_BLOCK) {
		const PageState* page_state = src.state ().page_state;
		const PageState* region_begin = page_state + end_page;
		const PageState* end = page_state + PAGES_PER_BLOCK;
		do {
			do {
				if (region_begin->VirtualAttributes.Win32Protection)
					break;
			} while (end > ++region_begin);
			if (region_begin < end) {
				const PageState* region_end = region_begin + 1;
				while (region_end < end) {
					if (!region_end->VirtualAttributes.Win32Protection)
						break;
					++region_end;
				}
				space_.protect ((Address)(address () + (region_begin - page_state) * PAGE_SIZE),
					(region_end - region_begin) * PAGE_SIZE, PAGE_NOACCESS);
				region_begin = region_end;
			}
		} while (region_begin < end);
	}
}

template <bool x64>
AddressSpace <x64>::AddressSpace (uint32_t process_id, HANDLE process_handle) :
	process_ (process_handle),
	mapping_ (nullptr),
	directory_ (nullptr)
{
	static const WCHAR fmt [] = OBJ_NAME_PREFIX WINWCS (".mmap.%08X");
	WCHAR name [_countof (fmt) + 8 - 3];
	wsprintfW (name, fmt, process_id);

	if (GetCurrentProcessId () == process_id) {
		LARGE_INTEGER size;
		size.QuadPart = (LONGLONG)(DIRECTORY_SIZE * sizeof (BlockInfo));
		mapping_ = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, name);
		if (!mapping_)
			throw_INITIALIZE ();
	} else {
		mapping_ = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, name);
		if (!mapping_)
			throw_INITIALIZE ();
	}

	if (x64)
		directory64_ = (BlockInfo**)VirtualAlloc (nullptr, (DIRECTORY_SIZE + SECOND_LEVEL_BLOCK - 1) / SECOND_LEVEL_BLOCK * sizeof (BlockInfo*), MEM_RESERVE, PAGE_READWRITE);
	else
		directory32_ = (BlockInfo*)MapViewOfFile (mapping_, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (!directory_) {
		verify (CloseHandle (mapping_));
		mapping_ = nullptr;
		throw_INITIALIZE ();
	}
}

template <bool x64>
AddressSpace <x64>::~AddressSpace () NIRVANA_NOEXCEPT
{
	if (directory_) {
		if (x64) {
			BlockInfo** end = directory64_ + (DIRECTORY_SIZE + SECOND_LEVEL_BLOCK - 1) / SECOND_LEVEL_BLOCK;
			for (BlockInfo** page = directory64_; page < end; page += PAGE_SIZE / sizeof (BlockInfo**)) {
				MEMORY_BASIC_INFORMATION mbi;
				verify (VirtualQuery (page, &mbi, sizeof (mbi)));
				if (mbi.State == MEM_COMMIT) {
					BlockInfo** end = page + PAGE_SIZE / sizeof (BlockInfo**);
					for (BlockInfo** p = page; p < end; ++p) {
						BlockInfo* block = *p;
						if (block) {
#ifdef _DEBUG
							if (GetCurrentProcess () == process_) {
								BYTE* address = (BYTE*)((p - directory64_) * SECOND_LEVEL_BLOCK * ALLOCATION_GRANULARITY);
								for (BlockInfo* page = block, *end = block + SECOND_LEVEL_BLOCK; page != end; page += PAGE_SIZE / sizeof (BlockInfo)) {
									verify (VirtualQuery (page, &mbi, sizeof (mbi)));
									if (mbi.State == MEM_COMMIT) {
										for (BlockInfo* p = page, *end = page + PAGE_SIZE / sizeof (BlockInfo); p != end; ++p, address += ALLOCATION_GRANULARITY) {
											HANDLE hm = p->mapping.handle ();
											if (INVALID_HANDLE_VALUE == hm) {
												VirtualFree (address, 0, MEM_RELEASE);
											} else {
												if (hm) {
													UnmapViewOfFile (address);
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
			verify (VirtualFree (directory64_, 0, MEM_RELEASE));
		} else {
#ifdef _DEBUG
			if (GetCurrentProcess () == process_) {
				BYTE* address = 0;
				for (BlockInfo* page = directory32_, *end = directory32_ + DIRECTORY_SIZE; page < end; page += PAGE_SIZE / sizeof (BlockInfo)) {
					MEMORY_BASIC_INFORMATION mbi;
					verify (VirtualQuery (page, &mbi, sizeof (mbi)));
					if (mbi.State == MEM_COMMIT) {
						for (BlockInfo* p = page, *end = page + PAGE_SIZE / sizeof (BlockInfo); p != end; ++p, address += ALLOCATION_GRANULARITY) {
							HANDLE hm = p->mapping.handle ();
							if (INVALID_HANDLE_VALUE == hm) {
								VirtualFree (address, 0, MEM_RELEASE);
							} else {
								if (hm) {
									UnmapViewOfFile (address);
									CloseHandle (hm);
								}
							}
						}
					} else
						address += PAGE_SIZE / sizeof (BlockInfo) * ALLOCATION_GRANULARITY;
				}
			}
#endif
			verify (UnmapViewOfFile (directory32_));
		}
		directory_ = nullptr;
	}
	if (mapping_) {
		verify (CloseHandle (mapping_));
		mapping_ = nullptr;
	}
}

template <bool x64>
BlockInfo* AddressSpace <x64>::block_no_commit (Address address)
{
	Size idx = (Size)address / ALLOCATION_GRANULARITY;
	assert (idx < DIRECTORY_SIZE);
	BlockInfo* p;
	if (x64) {
		Size i0 = idx / SECOND_LEVEL_BLOCK;
		Size i1 = idx % SECOND_LEVEL_BLOCK;
		if (!VirtualAlloc (directory64_ + i0, sizeof (BlockInfo*), MEM_COMMIT, PAGE_READWRITE))
			throw_NO_MEMORY ();
		BlockInfo** pp = directory64_ + i0;
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
	} else
		p = directory32_ + idx;

	return p;
}

template <bool x64>
BlockInfo& AddressSpace <x64>::block (Address address)
{
	BlockInfo* p = block_no_commit (address);
	if (!VirtualAlloc (p, sizeof (BlockInfo), MEM_COMMIT, PAGE_READWRITE))
		throw_NO_MEMORY ();
	return *p;
}

template <bool x64>
BlockInfo* AddressSpace <x64>::allocated_block (Address address)
{
	BlockInfo* p = nullptr;
	Size idx = (Size)address / ALLOCATION_GRANULARITY;
	if (idx < DIRECTORY_SIZE) {
		p = block_no_commit (address);
		MEMORY_BASIC_INFORMATION mbi;
		verify (VirtualQuery (p, &mbi, sizeof (mbi)));
		if (mbi.State != MEM_COMMIT || !p->mapping)
			p = nullptr;
	}
	return p;
}

template <bool x64>
typename AddressSpace <x64>::Address AddressSpace <x64>::reserve (Address dst, size_t& size, unsigned flags)
{
	if (!size)
		throw_BAD_PARAM ();

	if (sizeof (size_t) > sizeof (Size)) {
		if (size > std::numeric_limits <Size>::max ())
			throw_IMP_LIMIT ();
	}

	Address p;
	Address tgt;
	if (dst) {
		tgt = (Address)round_down ((Size)dst, (Size)ALLOCATION_GRANULARITY);
		if (flags & Memory::EXACTLY)
			size = round_up ((size_t)(dst - tgt + size), ALLOCATION_GRANULARITY);
		else
			size = round_up (size, ALLOCATION_GRANULARITY);
	} else {
		tgt = 0;
		size = round_up (size, ALLOCATION_GRANULARITY);
	}
	p = alloc (tgt, size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS);
	if (!p) {
		if (flags & Memory::EXACTLY)
			return 0;
		else
			throw_NO_MEMORY ();
	}

	{
		Address pb = p;
		try {
			for (Address end = p + (Size)size; pb < end; pb += ALLOCATION_GRANULARITY) {
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
			free (p, 0, MEM_RELEASE);
			if (flags & Memory::EXACTLY)
				return 0;
			throw;
		}
	}

	return p;
}

template <bool x64>
void AddressSpace <x64>::release (Address dst, size_t size)
{
	if (!(dst && size))
		return;

	Address const begin = (Address)round_down ((Size)dst, (Size)ALLOCATION_GRANULARITY);
	Address const end = (Address)round_up ((Size)(dst + size), (Size)ALLOCATION_GRANULARITY);

	// Check allocation and exclusive lock all released blocks.
	HANDLE begin_handle, end_handle;
	{
		BlockInfo* block = allocated_block (begin);
		if (!block)
			throw_FREE_MEM ();
		begin_handle = end_handle = block->mapping.exclusive_lock ();
		for (Address p = begin + ALLOCATION_GRANULARITY; p != end; p += ALLOCATION_GRANULARITY) {
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
		MBI begin_mbi = {0}, end_mbi = {0};
		if (INVALID_HANDLE_VALUE == begin_handle) {
			query (begin, begin_mbi);
			assert (MEM_RESERVE == begin_mbi.State);
			if ((Address)(uintptr_t)begin_mbi.BaseAddress + begin_mbi.RegionSize >= end)
				end_mbi = begin_mbi;
		}

		if (!end_mbi.BaseAddress) {
			Address back = end - PAGE_SIZE;
			if (INVALID_HANDLE_VALUE == end_handle) {
				query (back, end_mbi);
				assert (MEM_RESERVE == end_mbi.State);
			}
		}

		// Split reserved blocks at begin and end if need.
		if (begin_mbi.BaseAddress) {
			SSize realloc = begin - (Address)(uintptr_t)begin_mbi.AllocationBase;
			if (realloc > 0)
				verify (free ((Address)(uintptr_t)begin_mbi.AllocationBase, realloc, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER));
		}

		if (end_mbi.BaseAddress) {
			SSize realloc = (SSize )((Address)(uintptr_t)end_mbi.BaseAddress + end_mbi.RegionSize - end);
			if (realloc > 0)
				verify (free (end, realloc, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER));
		}
	}

	// Release memory
	for (Address p = begin; p < end;) {
		BlockInfo* block = allocated_block (p);
		HANDLE mapping = block->mapping.reset_and_unlock ();
		assert (mapping);
		if (INVALID_HANDLE_VALUE == mapping) {
			MBI mbi;
			query (p, mbi);
			assert (mbi.State == MEM_RESERVE);
			verify (free (p, 0, MEM_RELEASE));
			Address region_end = (Address)((uintptr_t)mbi.BaseAddress + mbi.RegionSize);
			if (region_end > end)
				region_end = end;
			p += ALLOCATION_GRANULARITY;
			while (p < region_end) {
				mapping = allocated_block (p)->mapping.reset_and_unlock ();
				assert (INVALID_HANDLE_VALUE == mapping);
				p += ALLOCATION_GRANULARITY;
			}
		} else {
			verify (unmap (p, 0));
			verify (CloseHandle (mapping));
			p += ALLOCATION_GRANULARITY;
		}
	}
}

template <bool x64>
void AddressSpace <x64>::check_allocated (Address ptr, size_t size)
{
	if (!size)
		return;
	if (!ptr)
		throw_BAD_PARAM ();

	for (Address p = ptr, end = p + (Size)size; p < end; p += ALLOCATION_GRANULARITY)
		if (!allocated_block (p))
			throw_BAD_PARAM ();
}

}
}
}

#endif
