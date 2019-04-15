// Nirvana project.
// Windows OS implementation.
// win32.h - base definitions for Win32 API.

#ifndef NIRVANA_CORE_WINDOWS_WIN32_H_
#define NIRVANA_CORE_WINDOWS_WIN32_H_

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// "interface" is used as identifier in ORB.
#ifdef interface
#undef interface
#endif

namespace Nirvana {
namespace Core {
namespace Windows {

/** Stack size for neutral fibers.
Neutral fiber performs limited number of calls: read from mailslot, extract other fibers 
from queue and remap stack memory. Default stack size for fibers is 1MB, but for neutral
fibers we can decrease stack size to save memory. If set to 0, neutral fibers have 
default stack size.
*/
const SIZE_T NEUTRAL_FIBER_STACK_SIZE = 0;

/// Worker thread priority should be greater than normal thread priority.
const int WORKER_THREAD_PRIORITY = THREAD_PRIORITY_ABOVE_NORMAL;

const DWORD SCHEDULER_ACK_TIMEOUT = 1000;

const SIZE_T PAGE_SIZE = 4096;
const SIZE_T PAGES_PER_BLOCK = 16; // Windows allocate memory by 64K blocks
const SIZE_T ALLOCATION_GRANULARITY = PAGE_SIZE * PAGES_PER_BLOCK;

#define OBJ_NAME_PREFIX L"Nirvana"

/// Thread information block
static const NT_TIB* current_TIB ()
{
#ifdef _M_IX86
	return (const NT_TIB*)__readfsdword (0x18);
#elif _M_AMD64
	return (const NT_TIB*)__readgsqword (0x30);
#else
#error Only x86 and x64 platforms supported.
#endif
}

/// Helper to detect is memory placed in current thread's stack.
inline bool is_current_stack (void* p)
{
	void* stack_begin = &stack_begin;
	return p >= stack_begin && p < current_TIB ()->StackBase;
}

}
}
}

#endif  //  _WIN32_H_
