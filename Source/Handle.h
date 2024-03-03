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
#ifndef NIRVANA_CORE_WINDOWS_HANDLE_H_
#define NIRVANA_CORE_WINDOWS_HANDLE_H_
#pragma once

#include <stdint.h>
#include <assert.h>

extern "C" __declspec (dllimport)
int __stdcall CloseHandle (void* handle);

namespace Nirvana {
namespace Core {
namespace Windows {

class Handle
{
public:
	Handle () noexcept :
		h_ (nullptr)
	{}

	Handle (void* h) noexcept :
		h_ (h)
	{}

	Handle (Handle&& src) noexcept :
		h_ (src.h_)
	{
		src.h_ = nullptr;
	}

	~Handle ()
	{
		if (is_valid ())
			CloseHandle (h_);
	}

	bool is_valid () const noexcept
	{
		return h_ && h_ != invalid ();
	}
	
	void close () noexcept
	{
		if (is_valid ()) {
			CloseHandle (h_);
			h_ = nullptr;
		}
	}

	operator void* () const noexcept
	{
		return h_;
	}

protected:
	static void* invalid () noexcept
	{
		return (void*)(intptr_t)-1;
	}

	void attach (HANDLE h) noexcept
	{
		assert (!is_valid ());
		h_ = h;
	}

private:
	void* h_;
};

}
}
}

#endif
