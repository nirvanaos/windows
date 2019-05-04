// Nirvana project
// Protection domain memory service over Win32 API

#include "../Port/ProtDomainMemory.h"
#include "AddressSpace.h"
#include <eh.h>
#include "ProtDomainMemoryBlock.h"
#include <Nirvana/real_copy.h>

namespace Nirvana {
namespace Core {
namespace Port {

using namespace ::Nirvana::Core::Windows;

Windows::AddressSpace ProtDomainMemory::space_;

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

inline HANDLE ProtDomainMemory::new_mapping ()
{
	HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_EXECUTE_READWRITE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, 0);
	if (!mapping)
		throw NO_MEMORY ();
	return mapping;
}


ULong ProtDomainMemory::commit_no_check (void* ptr, UWord size)
{
	ULong mask = 0;
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

void ProtDomainMemory::prepare_to_share (void* src, UWord size, Long flags)
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

void* ProtDomainMemory::allocate (void* dst, UWord size, Long flags)
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

void ProtDomainMemory::release (void* dst, UWord size)
{
	space_.release (dst, size);
}

void ProtDomainMemory::commit (void* ptr, UWord size)
{
	if (!size)
		return;

	if (!ptr)
		throw BAD_PARAM ();

	// Memory must be allocated.
	space_.check_allocated (ptr, size);

	commit_no_check (ptr, size);
}

void ProtDomainMemory::decommit (void* ptr, UWord size)
{
	space_.decommit (ptr, size);
}

ULong ProtDomainMemory::check_committed (void* ptr, UWord size)
{
	ULong state_bits = 0;
	for (const BYTE* begin = (const BYTE*)ptr, *end = begin + size; begin < end;) {
		MEMORY_BASIC_INFORMATION mbi;
		query (begin, mbi);
		if (!(mbi.Protect & PageState::MASK_ACCESS))
			throw BAD_PARAM ();
		state_bits |= mbi.Protect;
		begin = (const BYTE*)mbi.BaseAddress + mbi.RegionSize;
	}
	return state_bits;
}

void* ProtDomainMemory::copy (void* dst, void* src, UWord size, Long flags)
{
	if (!size)
		return dst;

	if (flags & ~(Memory::READ_ONLY | Memory::RELEASE | Memory::ALLOCATE | Memory::EXACTLY))
		throw INV_FLAG ();

	bool src_not_share, dst_not_share = false;
	// Source range have to be committed.
	
	ULong src_prot_mask;
	if (space_.allocated_block (src)) {
		src_prot_mask = space_.check_committed (src, size);
		src_not_share = is_current_stack (src);
	} else {
		src_prot_mask = check_committed (src, size);
		src_not_share = true;
	}

	void* ret = 0;
	UWord src_align = (UWord)src % ALLOCATION_GRANULARITY;
	try {
		if (!dst && Memory::RELEASE != (flags & Memory::RELEASE) && !src_not_share && round_up ((BYTE*)src + size, ALLOCATION_GRANULARITY) - (BYTE*)src <= ALLOCATION_GRANULARITY) {
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
				if (space_.allocated_block (dst)) {
					space_.check_allocated (dst, size);
					dst_not_share = is_current_stack (dst);
				} else {
					if (!is_writable (dst, size))
						throw BAD_PARAM ();
					dst_not_share = true;
				}
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
				if (!src_not_share && !dst_not_share && (UWord)ret % ALLOCATION_GRANULARITY == src_align) {
					// Share (regions may overlap).
					if (ret < src) {
						BYTE* pd = (BYTE*)ret, *end = pd + size;
						BYTE* ps = (BYTE*)src;
						if (end > src) {
							// Copy overlapped part with Memory::DECOMMIT.
							BYTE* first_part_end = round_up (end - ((BYTE*)src + size - end), ALLOCATION_GRANULARITY);
							assert (first_part_end < end);
							Long first_part_flags = (flags & ~Memory::RELEASE) | Memory::DECOMMIT;
							while (pd < first_part_end) {
								Block block (pd);
								BYTE* block_end = block.address () + ALLOCATION_GRANULARITY;
								UWord cb = block_end - pd;
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
							UWord cb = block_end - pd;
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
							Long first_part_flags = (flags & ~Memory::RELEASE) | Memory::DECOMMIT;
							while (pd > first_part_begin) {
								BYTE* block_begin = round_down (pd - 1, ALLOCATION_GRANULARITY);
								Block block (block_begin);
								UWord cb = pd - block_begin;
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
							UWord cb = pd - block_begin;
							ps -= cb;
							block.copy (ps, cb, flags);
							pd = block_begin;
						}
					}
				} else {
					// Physical copy.
					ULong state_bits;
					if (dst_not_share)
						state_bits = check_committed (dst, size);
					else
						state_bits = commit_no_check (ret, size);
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

UWord ProtDomainMemory::query (const void* p, Memory::QueryParam q)
{
	{
		switch (q) {

		case Memory::ALLOCATION_SPACE_BEGIN:
			{
				SYSTEM_INFO sysinfo;
				GetSystemInfo (&sysinfo);
				return (UWord)sysinfo.lpMinimumApplicationAddress;
			}

		case Memory::ALLOCATION_SPACE_END:
			return (UWord)space_.end ();

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
			return FLAGS;
		}

		throw BAD_PARAM ();
	}
}

bool ProtDomainMemory::is_readable (const void* p, UWord size)
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

bool ProtDomainMemory::is_private (const void* p, UWord size)
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

bool ProtDomainMemory::is_writable (const void* p, UWord size)
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

bool ProtDomainMemory::is_copy (const void* p, const void* plocal, UWord size)
{
	if ((UWord)p % ALLOCATION_GRANULARITY == (UWord)plocal % ALLOCATION_GRANULARITY) {
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

Long CALLBACK ProtDomainMemory::exception_filter (struct _EXCEPTION_POINTERS* pex)
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
