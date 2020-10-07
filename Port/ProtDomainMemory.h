// Nirvana project
// Windows implementation.
// Protection domain memory service over Win32 API

#ifndef NIRVANA_CORE_PORT_PROTDOMAINMEMORY_H_
#define NIRVANA_CORE_PORT_PROTDOMAINMEMORY_H_

#include <Nirvana/Memory.h>

struct _EXCEPTION_POINTERS;
typedef void *HANDLE;
typedef struct _MEMORY_BASIC_INFORMATION MEMORY_BASIC_INFORMATION;

namespace Nirvana {
namespace Core {

namespace Windows {
class ThreadMemory;
class AddressSpace;
}

namespace Port {

class ProtDomainMemory
{
	static const size_t PAGE_SIZE = 4096;
	static const size_t PAGES_PER_BLOCK = 16; // Windows allocate memory by 64K blocks
	static const size_t ALLOCATION_GRANULARITY = PAGE_SIZE * PAGES_PER_BLOCK;
public:
	static const size_t ALLOCATION_UNIT = ALLOCATION_GRANULARITY;
	static const size_t SHARING_UNIT = ALLOCATION_UNIT;
	static const size_t GRANULARITY = ALLOCATION_UNIT;
	static const size_t SHARING_ASSOCIATIVITY = ALLOCATION_UNIT;
	static const size_t OPTIMAL_COMMIT_UNIT = ALLOCATION_UNIT;

	static const size_t COMMIT_UNIT = PAGE_SIZE;
	static const size_t PROTECTION_UNIT = PAGE_SIZE;

	static const UWord FLAGS = 
		Memory::ACCESS_CHECK |
		Memory::HARDWARE_PROTECTION |
		Memory::COPY_ON_WRITE |
		Memory::SPACE_RESERVATION;

	static void initialize ();
	static void terminate ();

	// Memory::
	static void* allocate (void* dst, size_t size, UWord flags);

	static void release (void* dst, size_t size);

	static void commit (void* ptr, size_t size);

	static void decommit (void* ptr, size_t size);

	static void* copy (void* dst, void* src, size_t size, UWord flags);

	static bool is_readable (const void* p, size_t size);

	static bool is_writable (const void* p, size_t size);

	static bool is_private (const void* p, size_t size);

	static bool is_copy (const void* p1, const void* p2, size_t size);

	static uintptr_t query (const void* p, MemQuery q);

	//! For usage in proxies.
	static void prepare_to_share (void* src, size_t size, UWord flags);

	static long __stdcall exception_filter (struct _EXCEPTION_POINTERS* pex);
	static void se_translator (unsigned int, struct _EXCEPTION_POINTERS* pex);

private:
	friend class Windows::AddressSpace;
	friend class Windows::ThreadMemory;

	struct Region
	{
		void* ptr;
		size_t size;

		size_t subtract (void* begin, void* end)
		{
			uint8_t* my_end = (uint8_t*)ptr + size;
			if (ptr < begin) {
				if (my_end >= end)
					size = 0;
				if (my_end > begin)
					size -= my_end - (uint8_t*)begin;
			} else if (end >= my_end)
				size = 0;
			else if (end > ptr) {
				size -= (uint8_t*)end - (uint8_t*)ptr;
				ptr = end;
			}
			return size;
		}
	};

	class Block;

private:
	static uint32_t commit_no_check (void* ptr, size_t size);

	static void protect (void* address, size_t size, uint32_t protection);

	static void query (const void* address, MEMORY_BASIC_INFORMATION& mbi);

	static uint32_t check_committed (void* ptr, size_t size);

	static void change_protection (void* ptr, size_t size, UWord flags);

	// Create new mapping
	static HANDLE new_mapping ();

private:
	static Core::Windows::AddressSpace space_;
};

}
}
}

#endif  // _WIN_MEMORY_H_
