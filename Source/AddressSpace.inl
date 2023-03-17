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
#include "app_data.h"
#include <winioctl.h>

#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)

extern "C" {
	DWORD64 __cdecl X64Call (DWORD64 func, int argC, ...);
	DWORD64 __cdecl GetModuleHandle64 (const wchar_t* lpModuleName);
	DWORD64 __cdecl GetProcAddress64 (DWORD64 hModule, const char* funcName);
}

// Without the double casting, the pointer is sign extended, not zero extended,
// which leads to invalid addresses with /LARGEADDRESSAWARE.
#define PTR_TO_DWORD64(p) ((DWORD64)(ULONG_PTR)(p))

// Sign-extension is required for pseudo handles such as the handle returned
// from GetCurrentProcess().
// "64-bit versions of Windows use 32-bit handles for interoperability [...] it
// is safe to [...] sign-extend the handle (when passing it from 32-bit to
// 64-bit)."
// https://docs.microsoft.com/en-us/windows/win32/winprog64/interprocess-communication
#define HANDLE_TO_DWORD64(p) ((DWORD64)(LONG_PTR)(p))

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

inline bool address_space_init () NIRVANA_NOEXCEPT
{
	local_address_space.construct ();
	return local_address_space->initialize (GetCurrentProcessId (), GetCurrentProcess ());
}

inline void address_space_term () NIRVANA_NOEXCEPT
{
	local_address_space.destruct ();
}

inline void other_space_init ()
{
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	DWORD64 ntdll = GetModuleHandle64 (L"ntdll.dll");
	wow64_NtQueryVirtualMemory = GetProcAddress64 (ntdll, "NtQueryVirtualMemory");
	wow64_NtProtectVirtualMemory = GetProcAddress64 (ntdll, "NtProtectVirtualMemory");
	wow64_NtAllocateVirtualMemoryEx = GetProcAddress64 (ntdll, "NtAllocateVirtualMemoryEx");
	wow64_NtFreeVirtualMemory = GetProcAddress64 (ntdll, "NtFreeVirtualMemory");
	wow64_NtMapViewOfSectionEx = GetProcAddress64 (ntdll, "NtMapViewOfSectionEx");
	wow64_NtUnmapViewOfSectionEx = GetProcAddress64 (ntdll, "NtUnmapViewOfSectionEx");
#endif
}

template <bool x64> inline
void AddressSpace <x64>::query (Address address, MBI& mbi) const
{
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	if (x64) {
		DWORD64 ret = 0;
		DWORD64 status = X64Call (wow64_NtQueryVirtualMemory, 6, HANDLE_TO_DWORD64 (process_), address,
			(DWORD64)0, PTR_TO_DWORD64 (&mbi), (DWORD64)sizeof (mbi), PTR_TO_DWORD64 (&ret));
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
		assert (!status);
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
		DWORD64 status = X64Call (wow64_NtAllocateVirtualMemoryEx, 7, HANDLE_TO_DWORD64 (process_),
			PTR_TO_DWORD64 (&tmp_addr), PTR_TO_DWORD64 (&tmp_size), (DWORD64)flags, (DWORD64)protection,
			PTR_TO_DWORD64 (nullptr), (DWORD64)0);
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
		DWORD64 status = X64Call (wow64_NtFreeVirtualMemory, 4, HANDLE_TO_DWORD64 (process_),
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
		DWORD64 status = X64Call (wow64_NtUnmapViewOfSectionEx, 3, HANDLE_TO_DWORD64 (process_),
			(DWORD64)address, (DWORD64)flags);
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

template <bool x64> inline
void AddressSpace <x64>::close_mapping (HANDLE hm) const
{
	verify (DuplicateHandle (process_, hm, nullptr, nullptr, 0, FALSE, DUPLICATE_CLOSE_SOURCE));
}

template <bool x64>
void AddressSpace <x64>::Block::map (HANDLE mapping_map, HANDLE mapping_store, uint32_t protection)
{
	assert (mapping_map);
	assert (mapping_store);
	assert (INVALID_HANDLE_VALUE != mapping_map);
	assert (INVALID_HANDLE_VALUE != mapping_store);
	assert (exclusive_);

	HANDLE old = mapping ();
	assert (old);
	mapping (mapping_store);

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
		space_.close_mapping (old);
	}

	verify (space_.map (mapping_map, address (), ALLOCATION_GRANULARITY, MEM_REPLACE_PLACEHOLDER, protection));
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
		space_.close_mapping (hm);
		mapping (INVALID_HANDLE_VALUE);
	}
}

template <bool x64>
void AddressSpace <x64>::Block::copy (Port::Memory::Block& src, size_t offset, size_t size, uint32_t copied_pages_state)
{
	assert (exclusive_locked ());
	assert (size);
	assert (offset + size <= ALLOCATION_GRANULARITY);

	map_copy (src.mapping (), copied_pages_state);

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
void AddressSpace <x64>::Block::map_copy (HANDLE src_mapping, uint32_t protection)
{
	HANDLE hm;
	if (!DuplicateHandle (GetCurrentProcess (), src_mapping, space_.process (), &hm, 0, FALSE, DUPLICATE_SAME_ACCESS))
		throw_NO_MEMORY ();
	try {
		map (src_mapping, hm, protection);
	} catch (...) {
		space_.close_mapping (hm);
		throw;
	}
}

template <bool x64> inline
AddressSpace <x64>::AddressSpace () NIRVANA_NOEXCEPT :
	process_ (nullptr),
	file_ (INVALID_HANDLE_VALUE),
	mapping_ (nullptr),
	directory_ (nullptr)
{}

template <bool x64> inline
AddressSpace <x64>::AddressSpace (uint32_t process_id, HANDLE process_handle) :
	AddressSpace ()
{
	assert (GetCurrentProcessId () != process_id);
	if (!initialize (process_id, process_handle))
		throw_COMM_FAILURE ();
}

template <bool x64>
bool AddressSpace <x64>::initialize (uint32_t process_id, HANDLE process_handle) NIRVANA_NOEXCEPT
{
	process_ = process_handle;

	static const WCHAR fmt [] = WINWCS ("mmap%08X");

	bool local = GetCurrentProcessId () == process_id;
	WCHAR path [MAX_PATH + 1];
	size_t cc = get_app_data_path (path, std::size (path), local);
	if (!cc)
		return false;

	wsprintfW (path + cc, fmt, process_id);

	DWORD share, disposition, flags;
	if (local) {
		share = FILE_SHARE_READ | FILE_SHARE_WRITE;
		disposition = CREATE_ALWAYS;
		flags = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
	} else {
		share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		disposition = OPEN_EXISTING;
		flags = FILE_ATTRIBUTE_TEMPORARY;
	}

	file_ = CreateFileW (path, GENERIC_READ | GENERIC_WRITE, share, nullptr,
		disposition, flags, 0);

	if (INVALID_HANDLE_VALUE == file_)
		return false;

	LARGE_INTEGER file_size;

	if (local) {

		SYSTEM_INFO si;
		GetSystemInfo (&si);
		directory_size_ = (Size)((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY - 1)
			/ ALLOCATION_GRANULARITY;
		file_size.QuadPart = (LONGLONG)(directory_size_ * sizeof (BlockInfo));

		FILE_SET_SPARSE_BUFFER sb;
		sb.SetSparse = TRUE;
		DWORD cb;
		if (!DeviceIoControl (file_, FSCTL_SET_SPARSE, &sb, sizeof (sb), 0, 0, &cb, 0))
			return false;

		FILE_ZERO_DATA_INFORMATION zdi;
		zdi.FileOffset.QuadPart = 0;
		zdi.BeyondFinalZero = file_size;
		if (!DeviceIoControl (file_, FSCTL_SET_ZERO_DATA, &zdi, sizeof (zdi), 0, 0, &cb, 0))
			return false;

	} else {
		if (!GetFileSizeEx (file_, &file_size)) {
			CloseHandle (file_);
			file_ = INVALID_HANDLE_VALUE;
			return false;
		}
		directory_size_ = (Size)(file_size.QuadPart / sizeof (BlockInfo));
	}

	mapping_ = CreateFileMappingW (file_, nullptr, PAGE_READWRITE,
		file_size.HighPart, file_size.LowPart, nullptr);

	if (!mapping_) {
		CloseHandle (file_);
		file_ = INVALID_HANDLE_VALUE;
		return false;
	}

	if (x64)
		directory64_ = (BlockInfo**)VirtualAlloc (nullptr,
			(size_t)(directory_size_ + SECOND_LEVEL_BLOCK - 1) / SECOND_LEVEL_BLOCK * sizeof (BlockInfo*),
			MEM_RESERVE, PAGE_READWRITE);
	else
		directory32_ = (BlockInfo*)MapViewOfFile (mapping_, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (!directory_) {
		CloseHandle (mapping_);
		mapping_ = nullptr;
		CloseHandle (file_);
		file_ = INVALID_HANDLE_VALUE;
		return false;
	}

	return true;
}

template <bool x64>
AddressSpace <x64>::~AddressSpace () NIRVANA_NOEXCEPT
{
	if (directory_) {
		if (x64) {
			BlockInfo** end = directory64_ + (directory_size_ + SECOND_LEVEL_BLOCK - 1) / SECOND_LEVEL_BLOCK;
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
				for (BlockInfo* page = directory32_, *end = directory32_ + directory_size_; page < end; page += PAGE_SIZE / sizeof (BlockInfo)) {
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
		CloseHandle (mapping_);
		CloseHandle (file_);
	}
}

template <bool x64>
BlockInfo* AddressSpace <x64>::block_ptr (Address address, bool commit)
{
	BlockInfo* p = nullptr;
	Size idx = (Size)address / ALLOCATION_GRANULARITY;
	if (idx < directory_size_) {
		if (x64) {
			Size i0 = idx / SECOND_LEVEL_BLOCK;
			Size i1 = idx % SECOND_LEVEL_BLOCK;

			BlockInfo** pp = directory64_ + i0;
			if (commit) {
				if (!VirtualAlloc (pp, sizeof (BlockInfo*), MEM_COMMIT, PAGE_READWRITE))
					throw_NO_MEMORY ();
			} else {
				MEMORY_BASIC_INFORMATION mbi;
				verify (VirtualQuery (pp, &mbi, sizeof (mbi)));
				if (mbi.State != MEM_COMMIT)
					return nullptr;
			}
			p = *pp;
			if (!p && commit) {
				LARGE_INTEGER offset;
				offset.QuadPart = ALLOCATION_GRANULARITY * i0;
				Size blocks = directory_size_ - idx;
				if (blocks > SECOND_LEVEL_BLOCK)
					blocks = SECOND_LEVEL_BLOCK;
				p = (BlockInfo*)MapViewOfFile (mapping_, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, (size_t)blocks * sizeof (BlockInfo));
				if (!p)
					throw_NO_MEMORY ();
				BlockInfo* cur = (BlockInfo*)InterlockedCompareExchangePointer ((void* volatile*)pp, p, 0);
				if (cur) {
					UnmapViewOfFile (p);
					p = cur;
				}
			}
			if (p)
				p += i1;
		} else
			p = directory32_ + idx;
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

		if (!end_mbi.BaseAddress && INVALID_HANDLE_VALUE == end_handle) {
			Address back = end - PAGE_SIZE;
			query (back, end_mbi);
			assert (MEM_RESERVE == end_mbi.State);
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
			close_mapping (mapping);
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
