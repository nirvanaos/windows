/// \file
/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
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
#ifndef NIRVANA_CORE_WINDOWS_LOCKABLEHANDLE_H_
#define NIRVANA_CORE_WINDOWS_LOCKABLEHANDLE_H_

#include <Nirvana/Nirvana.h>
#include <BackOff.h>
#include <atomic>

namespace Nirvana {
namespace Core {
namespace Windows {

class LockableHandle
{
public:
	void init_invalid ()
	{
		assert (!val_);
		val_ = INVALID_VAL;
	}

	operator bool () const
	{
		return (val_ & ~SPIN_MASK) != 0;
	}

	void* lock ()
	{
		for (BackOff bo; true; bo ()) {
			uint32_t cur = val_.load ();
			while ((cur & SPIN_MASK) != SPIN_MASK) {
				if (val_.compare_exchange_weak (cur, cur + 1, std::memory_order_acquire))
					return val2handle (cur & ~SPIN_MASK);
			}
		}
	}

	void unlock ()
	{
		assert (val_ & SPIN_MASK);
		--val_;
	}

	void* exclusive_lock ()
	{
		for (BackOff bo; true; bo ()) {
			uint32_t cur = val_.load ();
			while ((cur & SPIN_MASK) == 0) {
				if (val_.compare_exchange_weak (cur, cur | SPIN_MASK, std::memory_order_acquire))
					return val2handle (cur);
			}
		}
	}

	void* handle () const
	{
		return val2handle (val_);
	}

	void set_and_unlock (void* handle)
	{
		// Must be exclusive locked
		assert ((val_ & SPIN_MASK) == SPIN_MASK);
		val_ = handle2val (handle);
	}

	HANDLE reset_and_unlock ()
	{
		// Must be exclusive locked
		assert ((val_ & SPIN_MASK) == SPIN_MASK);
		HANDLE h = val2handle (val_ & ~SPIN_MASK);
		val_ = 0;
		return h;
	}

	void exclusive_unlock ()
	{
		assert ((val_ & SPIN_MASK) == SPIN_MASK);
		val_.fetch_sub (SPIN_MASK, std::memory_order_release);
	}

	void reset_on_failure ()
	{
		val_ = 0;
	}

private:
	// Both 32-bit and 64-bit Windows use 32-bit handles for the interoperability.
	// Maximal number of handles per process is 0x1000000.
	// So SHIFT_BITS can be max 5. We limit it to 4 just in case.

	static const unsigned SHIFT_BITS = 4;

	// Low 2 bits of a handle are always zero.
	static const unsigned ALIGN_BITS = 2;

	static const unsigned LOCK_BITS = SHIFT_BITS + ALIGN_BITS;

	static const uint32_t SPIN_MASK = (uint32_t)~(~0 << LOCK_BITS);

	static const uint32_t INVALID_VAL = (uint32_t)(~0 << LOCK_BITS);

	static const uintptr_t INVALID_HANDLE = (uintptr_t)~0;

	static uint32_t handle2val (void* handle)
	{
		if (INVALID_HANDLE == (uintptr_t)handle)
			return INVALID_VAL;
		else {
			uintptr_t ui = (uintptr_t)handle;
			assert ((ui & ((~(uintptr_t)0 << (32 - SHIFT_BITS)) | ~(~(uintptr_t)0 << ALIGN_BITS))) == 0);
			return (uint32_t)ui << SHIFT_BITS;
		}
	}

	static void* val2handle (uint32_t val)
	{
		return INVALID_VAL == val ? (void*)INVALID_HANDLE : (void*)(uintptr_t)(val >> SHIFT_BITS);
	}

	volatile std::atomic <uint32_t> val_;
};

}
}
}

#endif
