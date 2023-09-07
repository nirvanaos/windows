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
#ifndef NIRVANA_CORE_PORT_FILEACCESSDIRECT_H_
#define NIRVANA_CORE_PORT_FILEACCESSDIRECT_H_
#pragma once

#include "../Source/FileAccess.h"
#include <IO_Request.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// Interface to host (kernel) filesystem driver.
class FileAccessDirect : 
	private Nirvana::Core::Windows::FileAccess
{
	typedef Nirvana::Core::Windows::FileAccess Base;

protected:
	typedef uint64_t Pos;      ///< File position type.
	typedef uint32_t Size;     ///< R/W block size type.
	typedef uint64_t BlockIdx; ///< Block index type. Must fit maximal position / minimal block_size.

	/// Constructor.
	/// 
	/// \param file A file.
	/// \param flags Creation flags.
	/// \param [out] size File size.
	/// \param [out] block_size Block (sector) size. 
	FileAccessDirect (File& file, unsigned flags, unsigned mode, Pos& size, Size& block_size);

	unsigned flags () const noexcept
	{
		return Base::flags ();
	}

	Ref <IO_Request> read (uint64_t pos, void* buf, uint32_t size);
	Ref <IO_Request> write (uint64_t pos, void* buf, uint32_t size);
	Ref <IO_Request> set_size (uint64_t size);
};

}
}
}

#endif
