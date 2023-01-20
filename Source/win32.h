/// \file
/// Base definitions for Win32 API.
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
#ifndef NIRVANA_CORE_WINDOWS_WIN32_H_
#define NIRVANA_CORE_WINDOWS_WIN32_H_
#pragma once

#include "WinWChar.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NO_STRICT

// We support Win 10 only.
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#define _ONECORE
#include <windows.h>

// "interface" is used as identifier in ORB.
#ifdef interface
#undef interface
#endif

#ifdef Yield
#undef Yield
#endif

#define OBJ_NAME_PREFIX WINWCS ("Nirvana")
#define MAILSLOT_PREFIX WINWCS ("\\\\.\\mailslot\\") OBJ_NAME_PREFIX WINWCS ("\\")

namespace Nirvana {
namespace Core {
namespace Windows {

/** Stack size for neutral fibers.
Neutral fiber performs limited number of calls: read from mailslot, extract other fibers 
from queue and remap stack memory. Default stack size for fibers is 1MB, but for neutral
fibers we can decrease stack size to save memory. If set to 0, neutral fibers have 
default stack size.
*/
const size_t NEUTRAL_FIBER_STACK_SIZE = 0x10000;

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

const size_t PAGE_SIZE = 4096;
const size_t PAGES_PER_BLOCK = 16; // Windows allocate memory by 64K blocks
const size_t ALLOCATION_GRANULARITY = PAGE_SIZE * PAGES_PER_BLOCK;

/// Signal exception numbers
const unsigned long STATUS_SIGNAL_BEGIN = 0xE0000000;

}
}
}

#endif  //  _WIN32_H_
