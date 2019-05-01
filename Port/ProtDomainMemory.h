// Nirvana project
// Windows implementation.
// Protection domain memory service over Win32 API

#ifndef NIRVANA_CORE_PORT_PROTDOMAINMEMORY_H_
#define NIRVANA_CORE_PORT_PROTDOMAINMEMORY_H_

#include "../Source/AddressSpace.h"
#include <Nirvana/Memory_s.h>
#include <Nirvana/real_copy.h>
#include <eh.h>

namespace Nirvana {
namespace Core {

namespace Windows {
class ThreadMemory;
}

namespace Port {

using namespace ::CORBA;
using namespace ::Nirvana::Core::Windows;

class ProtDomainMemory :
	public ::CORBA::Nirvana::ServantStatic <ProtDomainMemory, ::Nirvana::Memory>
{
public:
	static void initialize ()
	{
		space_.initialize ();
//		SetUnhandledExceptionFilter (&exception_filter);
		_set_se_translator (&se_translator);
	}

	static void terminate ()
	{
		SetUnhandledExceptionFilter (0);
		space_.terminate ();
	}

	static Memory_ptr singleton ()
	{
		return _this ();
	}

	// Memory::
	static void* allocate (void* dst, SIZE_T size, LONG flags)
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

	static void release (void* dst, SIZE_T size)
	{
		space_.release (dst, size);
	}

	static void commit (void* ptr, SIZE_T size)
	{
		if (!size)
			return;

		if (!ptr)
			throw BAD_PARAM ();

		// Memory must be allocated.
		space_.check_allocated (ptr, size);

		commit_no_check (ptr, size);
	}

private:
	static DWORD commit_no_check (void* ptr, SIZE_T size);

public:
	static void decommit (void* ptr, SIZE_T size)
	{
		space_.decommit (ptr, size);
	}

	static void* copy (void* dst, void* src, SIZE_T size, LONG flags);

	static bool is_readable (const void* p, SIZE_T size)
	{
		return space_.is_readable (p, size);
	}

	static bool is_writable (const void* p, SIZE_T size)
	{
		return space_.is_writable (p, size);
	}

	static bool is_private (const void* p, SIZE_T size)
	{
		return space_.is_private (p, size);
	}

	static bool is_copy (const void* p1, const void* p2, SIZE_T size)
	{
		return space_.is_copy (p1, p2, size);
	}

	static SIZE_T query (const void* p, Memory::QueryParam q);

	static void prepare_to_share (void* src, SIZE_T size, LONG flags);

	static LONG CALLBACK exception_filter (struct _EXCEPTION_POINTERS* pex);
	static void se_translator (unsigned int, struct _EXCEPTION_POINTERS* pex);

private:
	friend class Windows::AddressSpace;
	friend class Windows::ThreadMemory;

	struct Region
	{
		void* ptr;
		SIZE_T size;

		SIZE_T subtract (void* begin, void* end)
		{
			BYTE* my_end = (BYTE*)ptr + size;
			if (ptr < begin) {
				if (my_end >= end)
					size = 0;
				if (my_end > begin)
					size -= my_end - (BYTE*)begin;
			} else if (end >= my_end)
				size = 0;
			else if (end > ptr) {
				size -= (BYTE*)end - (BYTE*)ptr;
				ptr = end;
			}
			return size;
		}
	};

	class Block :
		public AddressSpace::Block
	{
	public:
		Block (void* addr) :
			AddressSpace::Block (ProtDomainMemory::space_, addr)
		{}

		DWORD commit (SIZE_T offset, SIZE_T size);
		bool need_remap_to_share (SIZE_T offset, SIZE_T size);

		void prepare_to_share (SIZE_T offset, SIZE_T size, LONG flags)
		{
			if (need_remap_to_share (offset, size))
				remap ();
			if (!(flags & Memory::DECOMMIT)) // Memory::RELEASE includes flag DECOMMIT.
				prepare_to_share_no_remap (offset, size);
		}

		void copy (void* src, SIZE_T size, LONG flags);

	private:
		struct Regions;
		class Remap;

		void remap ();
		bool copy_page_part (const void* src, SIZE_T size, LONG flags);
		void prepare_to_share_no_remap (SIZE_T offset, SIZE_T size);
		void copy (SIZE_T offset, SIZE_T size, const void* src, LONG flags);
	};

private:
	static void protect (void* address, SIZE_T size, DWORD protection)
	{
		//space_.protect (address, size, protection);
		DWORD old;
		verify (VirtualProtect (address, size, protection, &old));
	}

	static void query (const void* address, MEMORY_BASIC_INFORMATION& mbi)
	{
		//space_.query (address, mbi);
		verify (VirtualQuery (address, &mbi, sizeof (mbi)));
	}

	// Create new mapping
	static HANDLE new_mapping ()
	{
		HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_EXECUTE_READWRITE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, 0);
		if (!mapping)
			throw NO_MEMORY ();
		return mapping;
	}

private:
	static AddressSpace space_;
};

inline void* ProtDomainMemory::copy (void* dst, void* src, SIZE_T size, LONG flags)
{
	if (!size)
		return dst;

	if (flags & ~(Memory::READ_ONLY | Memory::RELEASE | Memory::ALLOCATE | Memory::EXACTLY))
		throw INV_FLAG ();

	// Source range have to be committed.
	DWORD src_prot_mask = space_.check_committed (src, size);

	// Current stack location
	void* stack_begin = &stack_begin;
	void* stack_end = current_TIB ()->StackBase;

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

inline SIZE_T ProtDomainMemory::query (const void* p, Memory::QueryParam q)
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

}
}
}

#endif  // _WIN_MEMORY_H_
