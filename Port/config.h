/// \file
/// Core compilation parameters
/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_CONFIG_H_
#define NIRVANA_CORE_CONFIG_H_
#pragma once

#include <Nirvana/time_defs.h>
#include <Nirvana/platform.h>

namespace Nirvana {
namespace Core {

/** \defgroup heap_parameters Heap parameters
@{
HEAP_UNIT is the minimum size of the allocated memory block,
also called the heap granularity.
The size of the allocated block is aligned up to this margin.
Thus, due to the size alignment, overhead costs are
HEAP_UNIT/2 bytes per allocated block.
In addition, the size of the bitmap is 2 bits per HEAP_UNIT heap bytes.
Thus, the optimal value of the HEAP_UNIT depends on the number and average
size of the allocated blocks.
In classic heap implementations, the overhead is usually at least 8 bytes
per allocated block. Modern object-oriented programs are characterized by a large
number of the quite small memory blocks. Thus, overhead costs in the classic heap
implementations are quite large.
In this implementation, the default block size is 16 bytes and average overhead
cost is 8 bytes per block.
User can create own heaps, see System::create_heap() API and specify heap granularity,
from HEAP_UNIT_MIN to HEAP_UNIT_MAX.
*/

/// Heap unit size by default, optimal
const size_t HEAP_UNIT_DEFAULT = sizeof (void*) * 4;

/// Minimal possible user heap granularity
const size_t HEAP_UNIT_MIN = 4;

/// Mmaximal possible user heap granularity
const size_t HEAP_UNIT_MAX = 4096;

/// Core heap granularity
const size_t HEAP_UNIT_CORE = HEAP_UNIT_DEFAULT;

/**	Size of the heap control block.
The size should be a multiple of the protection domain's heap granularity:
MAX (ALLOCATION_UNIT, PROTECTION_UNIT, SHARING_UNIT). If HEAP_DIRECTORY_SIZE
less than this value, the heap header contains several control blocks, and the heap itself
is divided into the corresponding number of parts, each of which works separately.
For Windows, the header size is 64K. For systems with smaller ALLOCATION_UNIT and SHARING_UNIT sizes
it can be made smaller.
*/
const size_t HEAP_DIRECTORY_SIZE = 0x10000;

/// See HeapDirectory.h
const size_t HEAP_DIRECTORY_LEVELS = 11;

/// @}

/** Maximum count of levels in the SkipList.
To provide best performance with a probabilistic time complexity of
O(logN) where N is the maximum number of elements, the skip list should
have SKIP_LIST_DEFAULT_LEVELS = logN. Too large value degrades the performance.
*/
const unsigned SKIP_LIST_DEFAULT_LEVELS = 10;

/// Unused module unloading timeout
/// 
/// If module was not used during this period of time, it will be unloaded.
const TimeBase::TimeT MODULE_UNLOAD_TIMEOUT = 10 * TimeBase::MINUTE;

/// Garbage collection deadline. Must be reasonable large.
/// May also be INFINITE_DEADLINE, but such a case will infinite delay GC
/// under stress environment.
const TimeBase::TimeT GC_DEADLINE = 10 * TimeBase::SECOND;

/// Disable iterator debugging.
/// May be used in production systems to eliminate unused runtime support code from core.
const bool RUNTIME_SUPPORT_DISABLE = false;

/// Execution domain creation may be heavy.
/// So we can enable pooling.
const bool EXEC_DOMAIN_POOLING = true;
const unsigned EXEC_DOMAIN_POOL_MIN = 10;

/// Usage of the background threads may be completely disabled.
const bool BACKGROUND_THREAD_DISABLE = false;

/// Background thread creation may be heavy.
/// So we can enable pooling.
const bool BACKGROUND_THREAD_POOLING = true;
const unsigned BACKGROUND_THREAD_POOL_MIN = 8;

const bool HEAP_POOLING = true;
const unsigned HEAP_POOL_MIN = 16;

/// Object pool housekeeping
const TimeBase::TimeT OBJECT_POOL_SHRINK_PERIOD = 5 * TimeBase::SECOND;

///@{
/// When a request is issued, the request deadline is not known yet.
/// So initially request is issued with some small deadline and then deadline is adjusted.

/// Initial deadline for the local system requests.
const TimeBase::TimeT INITIAL_REQUEST_DEADLINE_LOCAL = 1 * TimeBase::MICROSECOND;

/// Initial deadline for the remote requests.
const TimeBase::TimeT INITIAL_REQUEST_DEADLINE_REMOTE = 1 * TimeBase::MILLISECOND;

///@}

/// Cancel request deadline.
const TimeBase::TimeT CANCEL_REQUEST_DEADLINE = 1 * TimeBase::SECOND;

/// Close connection deadline.
const TimeBase::TimeT CLOSE_CONNECTION_DEADLINE = 1 * TimeBase::SECOND;

/// `true` if the implementation does not support protection domains.
const bool SINGLE_DOMAIN = false;

/// `true` to exclude system domain code from the binary.
/// 
/// Used for build auxiliary platform core.
const bool BUILD_NO_SYS_DOMAIN = HOST_PLATFORM != PLATFORM;

/// `true` to use atomic uint64_t for LockablePtr on 32-bit platform, if available.
/// `false` may be used for debugging native 32-bit.
const bool USE_LOCKABLE_PTR_64 = true;

/// For some host implementations, MemContext may be released out of the execution domain.
/// In this case we create async call for this.
const bool ENABLE_MEM_CONTEXT_ASYNC_DESTROY = false;

}
}

#endif
