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
#ifndef NIRVANA_CORE_WINDOWS_BUFFERPOOL_H_
#define NIRVANA_CORE_WINDOWS_BUFFERPOOL_H_
#pragma once

#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

/// BufferPool class.
class BufferPool
{
private:
	/// Deprecated
	BufferPool (const BufferPool&) = delete;
	/// Deprecated
	BufferPool& operator = (const BufferPool&) = delete;

public:
	BufferPool (size_t buffer_count, size_t buffer_size) NIRVANA_NOEXCEPT;
	~BufferPool () NIRVANA_NOEXCEPT;

	/// Returns data buffer pointer for specified OVERLAPPED pointer.
	static void* data (OVERLAPPED* ovl) NIRVANA_NOEXCEPT
	{
		return ovl + 1;
	}

protected:
	size_t buffer_size () const NIRVANA_NOEXCEPT
	{
		return buffer_size_;
	}

	OVERLAPPED* begin () const NIRVANA_NOEXCEPT
	{
		return begin_;
	}

	OVERLAPPED* end () const NIRVANA_NOEXCEPT
	{
		return end_;
	}

	OVERLAPPED* next (OVERLAPPED* ovl) const NIRVANA_NOEXCEPT
	{
		return (OVERLAPPED*)((BYTE*)data (ovl) + buffer_size_);
	}

private:
	OVERLAPPED* begin_;
	OVERLAPPED* end_;
	size_t buffer_size_;
};

}
}
}

#endif
