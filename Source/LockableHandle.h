/// \file
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
#ifndef NIRVANA_CORE_WINDOWS_LOCKABLEHANDLE_H_
#define NIRVANA_CORE_WINDOWS_LOCKABLEHANDLE_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <BackOff.h>
#include <atomic>

typedef void* HANDLE;

namespace Nirvana {
namespace Core {
namespace Windows {

class LockableHandle
{
public:
	void init_invalid () noexcept
	{
		// Must be null and exclusively locked
		assert (val_ == LOCK_MASK);
		// Must be lock-free
		assert (val_.is_lock_free ());
		val_.store (INVALID_VAL, std::memory_order_relaxed);
	}
	
	operator bool () const noexcept
	{
		return (val_ & ~LOCK_MASK) != 0;
	}
	
	void* lock () noexcept;

	void unlock () noexcept
	{
		assert (val_ & LOCK_MASK);
		val_.fetch_sub (LOCK_INC, std::memory_order_release);
	}

	void* exclusive_lock () noexcept;

	void exclusive_unlock () noexcept
	{
		assert ((val_ & LOCK_MASK) == LOCK_MASK);
		val_.fetch_sub (LOCK_MASK, std::memory_order_release);
	}

	void set_and_unlock (void* handle) noexcept
	{
		// Must be exclusive locked
		assert ((val_ & LOCK_MASK) == LOCK_MASK);
		val_ = handle2val (handle);
	}

	HANDLE reset_and_unlock () noexcept
	{
		// Must be exclusive locked
		assert ((val_ & LOCK_MASK) == LOCK_MASK);
		HANDLE h = val2handle (val_.load (std::memory_order_relaxed));
		val_.store (0, std::memory_order_release);
		return h;
	}

	void reset_on_failure () noexcept
	{
		assert ((val_ & LOCK_MASK) == LOCK_MASK);
		val_.store (0, std::memory_order_release);
	}

	void* handle () const noexcept
	{
		return val2handle (val_);
	}

private:
	// Both 32-bit and 64-bit Windows use 32-bit handles for the interoperability.

	// For all platforms of the given host platform IntegralType must be lock-free.
	// Now we use 32-bit type. This provides 6 bits for the lock counter.
	// For the future systems with extremely large number of cores we can use uint64_t
	// to extend lock counter capacity.
	using IntegralType = uint32_t;

	// Maximal number of handles per process is 0x1000000.
	static const unsigned USED_BITS = 25;

	// Low 2 bits of a handle are always zero.
	static const unsigned ALIGN_BITS = 2;

	// Only these bits in the valid Windows handle can be not zero.
	static const uintptr_t USED_BIT_MASK = ~(~(uintptr_t)0 << USED_BITS) << ALIGN_BITS;

	// Reserve 1 bit more for INVALID_HANDLE_VALUE
	static const unsigned LOCK_OFFSET = USED_BITS + 1;

	// Highest bits are used for the locking
	static const IntegralType LOCK_MASK = ~(IntegralType)0 << LOCK_OFFSET;
	static const IntegralType LOCK_INC = 1 << LOCK_OFFSET;

	// INVALID_HANDLE_VALUE
	static const IntegralType INVALID_VAL = ~LOCK_MASK;

	static IntegralType handle2val (void* handle) noexcept
	{
		if ((void*)(-1) == handle)
			return INVALID_VAL;
		else {
			assert (((uintptr_t)handle & ~USED_BIT_MASK) == 0);
			IntegralType ui = (IntegralType)(uintptr_t)handle;
			return ui >> ALIGN_BITS;
		}
	}

	static void* val2handle (IntegralType val) noexcept
	{
		val &= ~LOCK_MASK;
		if (INVALID_VAL == val)
			return (void*)(-1);
		else {
			uintptr_t ui = (uintptr_t)val << ALIGN_BITS;
			assert (!(ui & ~USED_BIT_MASK));
			return (void*)ui;
		}
	}

	class BackOff
	{
	public:
		BackOff ();

		void operator () ();

	private:
		unsigned iterations_;
		RandomGen rndgen_;
	};

	std::atomic <IntegralType> val_;
};

}
}
}

#endif
