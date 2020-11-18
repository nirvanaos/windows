#ifndef NIRVANA_CORE_WINDOWS_MEMORY_INL_
#define NIRVANA_CORE_WINDOWS_MEMORY_INL_

#include "../Port/Memory.h"
#include "AddressSpace.h"

namespace Nirvana {
namespace Core {
namespace Port {

class Memory::AddressSpace : public Windows::AddressSpace
{
public:
	AddressSpace () :
		Windows::AddressSpace (GetCurrentProcessId (), GetCurrentProcess ())
	{
		AddVectoredExceptionHandler (TRUE, &exception_filter);
	}
};

inline void Memory::protect (void* address, size_t size, uint32_t protection)
{
	//space_.protect (address, size, protection);
	DWORD old;
	verify (VirtualProtect (address, size, protection, &old));
}

class Memory::Block :
	public Core::Windows::AddressSpace::Block
{
public:
	Block (void* addr, bool exclusive = false) :
		Core::Windows::AddressSpace::Block (Memory::space_, addr, exclusive)
	{}

	DWORD commit (size_t offset, size_t size);
	bool need_remap_to_share (size_t offset, size_t size);

	void prepare_to_share (size_t offset, size_t size, UWord flags)
	{
		if (need_remap_to_share (offset, size))
			if (exclusive_lock () && need_remap_to_share (offset, size))
				remap ();
		if (!(flags & Nirvana::Memory::DECOMMIT)) // Memory::RELEASE includes flag DECOMMIT.
			prepare_to_share_no_remap (offset, size);
	}

	void aligned_copy (void* src, size_t size, UWord flags);
	void copy (size_t offset, size_t size, const void* src, UWord flags);

	void decommit (size_t offset, size_t size);
	DWORD check_committed (size_t offset, size_t size);

	void change_protection (size_t offset, size_t size, UWord flags);

	bool is_copy (Block& other, size_t offset, size_t size);

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

