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

#define OBJ_NAME_PREFIX L"Nirvana"
#define MAILSLOT_PREFIX L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\"

namespace Nirvana {
namespace Core {
namespace Windows {

/** Stack size for neutral fibers.
Neutral fiber performs limited number of calls: read from mailslot, extract other fibers 
from queue and remap stack memory. Default stack size for fibers is 1MB, but for neutral
fibers we can decrease stack size to save memory. If set to 0, neutral fibers have 
default stack size.
*/
const SIZE_T NEUTRAL_FIBER_STACK_SIZE = 0; // TODO: Decrease stack size.

/// Worker thread priority should be greater than normal thread priority.
const int WORKER_THREAD_PRIORITY = THREAD_PRIORITY_ABOVE_NORMAL;

/// Background thread priority is normal priority.
const int BACKGROUND_THREAD_PRIORITY = THREAD_PRIORITY_NORMAL;

/// Boosted background thread priority must be above worker thread priority.
const int BACKGROUND_THREAD_PRIORITY_BOOSTED = THREAD_PRIORITY_HIGHEST;

/// System scheduler threads must have maximal priority.
const int SCHEDULER_THREAD_PRIORITY = THREAD_PRIORITY_TIME_CRITICAL;

/// Postman thread priority must be above worker thread priority.
const int POSTMAN_THREAD_PRIORITY = THREAD_PRIORITY_HIGHEST;

const DWORD PROCESS_START_ACK_TIMEOUT = 1000; // 1 sec

const SIZE_T PAGE_SIZE = 4096;
const SIZE_T PAGES_PER_BLOCK = 16; // Windows allocate memory by 64K blocks
const SIZE_T ALLOCATION_GRANULARITY = PAGE_SIZE * PAGES_PER_BLOCK;

}
}
}

#endif  //  _WIN32_H_
