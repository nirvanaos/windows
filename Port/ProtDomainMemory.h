// Nirvana project
// Windows implementation.
// Protection domain memory service over Win32 API

#ifndef NIRVANA_CORE_PORT_PROTDOMAINMEMORY_H_
#define NIRVANA_CORE_PORT_PROTDOMAINMEMORY_H_

#include <Nirvana/Memory_c.h>
#include "../Source/AddressSpace.h"
#include <Nirvana/real_copy.h>
#include <eh.h>

namespace Nirvana {
namespace Core {

namespace Windows {
class ThreadMemory;
class AddressSpace;
}

namespace Port {

using namespace ::CORBA;
using namespace ::Nirvana::Core::Windows;

class ProtDomainMemory
{
public:
	static void initialize ();
	static void terminate ();

	// Memory::
	static void* allocate (void* dst, SIZE_T size, LONG flags);

	static void release (void* dst, SIZE_T size);

	static void commit (void* ptr, SIZE_T size);

	static void decommit (void* ptr, SIZE_T size);

	static void* copy (void* dst, void* src, SIZE_T size, LONG flags);

	static bool is_readable (const void* p, SIZE_T size);

	static bool is_writable (const void* p, SIZE_T size);

	static bool is_private (const void* p, SIZE_T size);

	static bool is_copy (const void* p1, const void* p2, SIZE_T size);

	static SIZE_T query (const void* p, Memory::QueryParam q);

	//! For usage in proxies.
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
	static DWORD commit_no_check (void* ptr, SIZE_T size);

	static void protect (void* address, SIZE_T size, DWORD protection);

	static void query (const void* address, MEMORY_BASIC_INFORMATION& mbi);

	// Create new mapping
	static HANDLE new_mapping ();

private:
	static AddressSpace space_;
};

}
}
}

#endif  // _WIN_MEMORY_H_
