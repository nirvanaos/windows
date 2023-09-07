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
#ifndef NIRVANA_CORE_PORT_REQUESTOVERLAPPED_H_
#define NIRVANA_CORE_PORT_REQUESTOVERLAPPED_H_
#pragma once

#include <IO_Request.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class RequestOverlapped :
	public IO_Request,
	public OVERLAPPED
{
public:
	RequestOverlapped (HANDLE file) noexcept :
		file_ (file)
	{
		zero (static_cast <OVERLAPPED&> (*this));
	}

	RequestOverlapped (HANDLE file, uint64_t offset) noexcept :
		file_ (file)
	{
		zero (static_cast <OVERLAPPED&> (*this));
		OffsetHigh = offset >> 32;
		Offset = (DWORD)offset;
	}

	static RequestOverlapped& from_overlapped (OVERLAPPED& ovl) noexcept
	{
		return static_cast <RequestOverlapped&> (ovl);
	}

	virtual void cancel () noexcept override
	{
		CancelIoEx (file_, this);
	}

private:
	HANDLE file_;
};

}
}
}

#endif
