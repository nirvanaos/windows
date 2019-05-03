// Nirvana project
// Windows implementation.
// Protection domain memory service over Win32 API

#ifndef NIRVANA_CORE_PORT_PROTDOMAINMEMORY_H_
#define NIRVANA_CORE_PORT_PROTDOMAINMEMORY_H_

#include <Nirvana/Memory_c.h>

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

using namespace ::CORBA;
using namespace ::Nirvana::Core::Windows;

class ProtDomainMemory
{
public:
	static void initialize ();
	static void terminate ();

	// Memory::
	static void* allocate (void* dst, UWord size, Long flags);

	static void release (void* dst, UWord size);

	static void commit (void* ptr, UWord size);

	static void decommit (void* ptr, UWord size);

	static void* copy (void* dst, void* src, UWord size, Long flags);

	static bool is_readable (const void* p, UWord size);

	static bool is_writable (const void* p, UWord size);

	static bool is_private (const void* p, UWord size);

	static bool is_copy (const void* p1, const void* p2, UWord size);

	static UWord query (const void* p, Memory::QueryParam q);

	//! For usage in proxies.
	static void prepare_to_share (void* src, UWord size, Long flags);

	static Long __stdcall exception_filter (struct _EXCEPTION_POINTERS* pex);
	static void se_translator (unsigned int, struct _EXCEPTION_POINTERS* pex);

private:
	friend class Windows::AddressSpace;
	friend class Windows::ThreadMemory;

	struct Region
	{
		void* ptr;
		UWord size;

		UWord subtract (void* begin, void* end)
		{
			Octet* my_end = (Octet*)ptr + size;
			if (ptr < begin) {
				if (my_end >= end)
					size = 0;
				if (my_end > begin)
					size -= my_end - (Octet*)begin;
			} else if (end >= my_end)
				size = 0;
			else if (end > ptr) {
				size -= (Octet*)end - (Octet*)ptr;
				ptr = end;
			}
			return size;
		}
	};

	class Block;

private:
	static ULong commit_no_check (void* ptr, UWord size);

	static void protect (void* address, UWord size, ULong protection);

	static void query (const void* address, MEMORY_BASIC_INFORMATION& mbi);

	static ULong check_committed (void* ptr, UWord size);

	// Create new mapping
	static HANDLE new_mapping ();

private:
	static AddressSpace space_;
};

}
}
}

#endif  // _WIN_MEMORY_H_
