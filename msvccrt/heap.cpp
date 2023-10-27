#include <Nirvana/c_heap_dbg.h>
#include <stdlib.h>

using namespace Nirvana;

extern "C" {

void* _recalloc (void* p, size_t count, size_t element_size)
{
	if (std::numeric_limits <size_t>::max () / element_size < count)
		return nullptr;
	return realloc (p, count * element_size);
}

void* _malloc_base (size_t size)
{
	return malloc (size);
}

void* _calloc_base (size_t num, size_t size)
{
	return calloc (num, size);
}

void _free_base (void* p)
{
	free (p);
}

void* _realloc_base (void* p, size_t size)
{
	return realloc (p, size);
}

#ifdef _DEBUG

void* _malloc_dbg (size_t size, int block_type, const char* file_name, int line_number)
{
	return Nirvana::c_malloc <HeapBlockHdrDbg> (size, block_type, file_name, line_number);
}

void* _calloc_dbg (
	size_t      count,
	size_t      element_size,
	int         block_type,
	const char* file_name,
	int         line_number
) {
	return Nirvana::c_calloc <HeapBlockHdrDbg> (count, element_size, block_type, file_name, line_number);
}

void* _realloc_dbg (void* p, size_t size, int block_type, const char* file_name, int line_number)
{
	return Nirvana::c_realloc <HeapBlockHdrDbg> (p, size, block_type, file_name, line_number);
}

void* _recalloc_dbg (
	void*       p,
	size_t      count,
	size_t      element_size,
	int         block_type,
	const char* file_name,
	int         const line_number
) {
	if (std::numeric_limits <size_t>::max () / element_size < count)
		return nullptr;
	return Nirvana::c_realloc <HeapBlockHdrDbg> (p, count * element_size, block_type, file_name, line_number);
}

void _free_dbg (void* p, int block_type)
{
	Nirvana::c_free <HeapBlockHdrDbg> (p, block_type);
}

#endif

}

extern "C" int __cdecl _callnewh (size_t const size)
{
	return 0;
}
