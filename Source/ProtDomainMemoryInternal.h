#ifndef NIRVANA_CORE_WINDOWS_PROTDOMAINMEMORYINTERNAL_H_
#define NIRVANA_CORE_WINDOWS_PROTDOMAINMEMORYINTERNAL_H_

#include "../Port/ProtDomainMemory.h"
#include "AddressSpace.h"
#include <Runnable.h>

namespace Nirvana {
namespace Core {
namespace Port {

inline void ProtDomainMemory::protect (void* address, size_t size, uint32_t protection)
{
	//space_.protect (address, size, protection);
	DWORD old;
	verify (VirtualProtect (address, size, protection, &old));
}

class ProtDomainMemory::Block :
	public Core::Windows::AddressSpace::Block
{
public:
	Block (void* addr) :
		Core::Windows::AddressSpace::Block (ProtDomainMemory::space_, addr)
	{}

	DWORD commit (size_t offset, size_t size);
	bool need_remap_to_share (size_t offset, size_t size);

	void prepare_to_share (size_t offset, size_t size, UWord flags)
	{
		if (need_remap_to_share (offset, size))
			remap ();
		if (!(flags & Memory::DECOMMIT)) // Memory::RELEASE includes flag DECOMMIT.
			prepare_to_share_no_remap (offset, size);
	}

	void aligned_copy (void* src, size_t size, UWord flags);
	void copy (size_t offset, size_t size, const void* src, UWord flags);

private:
	struct Regions;

	void remap ();
	void prepare_to_share_no_remap (size_t offset, size_t size);
};

struct ProtDomainMemory::Block::Regions
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

