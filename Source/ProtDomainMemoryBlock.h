#ifndef NIRVANA_CORE_WINDOWS_PROTDOMAINMEMORYBLOCK_H_
#define NIRVANA_CORE_WINDOWS_PROTDOMAINMEMORYBLOCK_H_

#include "../Port/ProtDomainMemory.h"
#include "AddressSpace.h"
#include "RunnableImpl.h"

namespace Nirvana {
namespace Core {
namespace Port {

inline void ProtDomainMemory::protect (void* address, UWord size, ULong protection)
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

	ULong commit (UWord offset, UWord size);
	bool need_remap_to_share (UWord offset, UWord size);

	void prepare_to_share (UWord offset, UWord size, Long flags)
	{
		if (need_remap_to_share (offset, size))
			remap ();
		if (!(flags & Memory::DECOMMIT)) // Memory::RELEASE includes flag DECOMMIT.
			prepare_to_share_no_remap (offset, size);
	}

	void copy (void* src, UWord size, Long flags);

private:
	struct Regions;
	class Remap;

	void remap ();
	bool copy_page_part (const void* src, UWord size, Long flags);
	void prepare_to_share_no_remap (UWord offset, UWord size);
	void copy (UWord offset, UWord size, const void* src, Long flags);
};

struct ProtDomainMemory::Block::Regions
{
	Region begin [PAGES_PER_BLOCK];
	Region* end;

	Regions () :
		end (begin)
	{}

	void add (void* ptr, UWord size)
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

}
}
}

#endif

