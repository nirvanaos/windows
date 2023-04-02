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
#include "LockableHandle.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void* LockableHandle::lock () NIRVANA_NOEXCEPT
{
	for (BackOff bo; true; bo ()) {
		uint32_t cur = val_.load ();
		while ((cur & SPIN_MASK) != SPIN_MASK) {
			if (val_.compare_exchange_weak (cur, cur + 1, std::memory_order_acquire))
				return val2handle (cur & ~SPIN_MASK);
		}
	}
}

void* LockableHandle::exclusive_lock () NIRVANA_NOEXCEPT
{
	for (BackOff bo; true; bo ()) {
		uint32_t cur = val_.load ();
		while ((cur & SPIN_MASK) == 0) {
			if (val_.compare_exchange_weak (cur, cur | SPIN_MASK, std::memory_order_acquire))
				return val2handle (cur);
		}
	}
}

}
}
}
