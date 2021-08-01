/// \file
/// Compile configuration parameters

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

#include <stdint.h>

namespace Nirvana {
namespace Core {

// Heap parameters

/*
	HEAP_UNIT - минимальный размер выделяемого блока памяти по умолчанию.
	Размер выделяемого блока выравнивается в сторону увеличения на эту величину.
	Таким образом, за счет выравнивания, накладные расходы составляют
	HEAP_UNIT/2 байт на каждый выделенный блок.
	Кроме того, размер битовой карты составляет 2 бита на HEAP_UNIT байт кучи.
	Таким образом, оптимальная величина HEAP_UNIT зависит от количества и среднего
	размера выделенных блоков.
	В классических реализациях кучи, накладные расходы составляют обычно не менее 8 байт
	на выделенный блок. Для современных объектно-ориентированных программ характерно большое
	количество небольших блоков памяти. Таким образом, накладные расходы в обычной
	куче достаточно велики.
	В данной реализации размер блока по умолчанию равен 16 байт.
*/

const size_t HEAP_UNIT_MIN = 4;
const size_t HEAP_UNIT_DEFAULT = 16;
const size_t HEAP_UNIT_CORE = 16; // Core heap unit.
const size_t HEAP_UNIT_MAX = 4096;

/**	Размер управляющего блока кучи. 
Размер должен быть кратен гранулярности памяти домена защиты - максимальному
значению MAX (ALLOCATION_UNIT, PROTECTION_UNIT, SHARING_UNIT). Если HEAP_DIRECTORY_SIZE
меньше этой величины, заголовок кучи содержит несколько управляющих блоков, а сама куча
делится на соответствующее количество частей, каждая из которых работает отдельно.
Для Windows размер заголовка равен 64K. Для систем с меньшими размерами ALLOCATION_UNIT
и SHARING_UNIT его можно сделать меньше.
*/
const size_t HEAP_DIRECTORY_SIZE = 0x10000;
const size_t HEAP_DIRECTORY_LEVELS = 11;

/** Heap directory implementation.
COMMITTED_BITMAP commits all bitmap memory on heap initialization.
RESERVED_BITMAP_WITH_EXCEPTIONS conserves the physical memory usage but slightly reduces
the performance. RESERVED_BITMAP_WITH_EXCEPTIONS is not supported by Clang.
RESERVED_BITMAP is extremely slow and unusable.
PLAIN_MEMORY provides the best performance but wastes a lot of physical memory.
*/

#if !defined (__clang__)
//#define HEAP_DIRECTORY_IMPLEMENTATION HeapDirectoryImpl::RESERVED_BITMAP_WITH_EXCEPTIONS
#define HEAP_DIRECTORY_IMPLEMENTATION HeapDirectoryImpl::COMMITTED_BITMAP
#else
#define HEAP_DIRECTORY_IMPLEMENTATION HeapDirectoryImpl::COMMITTED_BITMAP
#endif

/** Maximum count of levels in PriorityQueue.
To provide best performance with a probabilistic time complexity of
O(logN) where N is the maximum number of elements, the queue should
have PRIORITY_QUEUE_LEVELS = logN. Too large value degrades the performance.
*/
const unsigned SYNC_DOMAIN_PRIORITY_QUEUE_LEVELS = 10; //!< For syncronization domain.
const unsigned SYS_DOMAIN_PRIORITY_QUEUE_LEVELS = 10; //!< For system-wide scheduler.
const unsigned PROT_DOMAIN_PRIORITY_QUEUE_LEVELS = 10; //!< For protection domain scheduler.

typedef uint32_t ProtDomainId;
typedef uint32_t ObjRefSignature;
typedef uint32_t UserToken;

/// Unused module unloading timeout
/// 
/// If module was not used during this period of time, it will be unloaded.
const uint64_t MODULE_UNLOAD_TIMEOUT = 30000000000; // 30 sec

/// ORB proxy garbage collection deadline. May be INFINITE_DEADLINE
const uint64_t PROXY_GC_DEADLINE = 10000000000; // 10 sec

}
}

#endif
