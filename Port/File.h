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
#ifndef NIRVANA_CORE_PORT_FILE_H_
#define NIRVANA_CORE_PORT_FILE_H_
#pragma once

#include "../Source/DirItem.h"

namespace Nirvana {
namespace Core {

namespace Windows {
class FileAccess;
}

namespace Port {

class FileAccessDirect;

/// Implements File object for host system.
class File : private Windows::DirItem
{
	typedef Windows::DirItem Base;

	///@{
	/// DirItem implementation.

public:
	/// File system object type.
	FileType type () noexcept;

	/// Existance check
	bool _non_existent () noexcept
	{
		if (type () == FileType::not_found)
			return true;
		return Base::_non_existent ();
	}

	/// File system object status information.
	/// 
	/// \param [out] st FileStat structure.
	void stat (FileStat& st)
	{
		// File type can be unknown here, check.
		if (type () == FileType::not_found)
			throw CORBA::OBJECT_NOT_EXIST (ENOENT);
		Base::stat (st);
	}

protected:
	/// File system object unique id.
	const DirItemId& id () const noexcept
	{
		return Base::id ();
	}

	/// Etherealize file system object: close all handles etc.
	void etherealize () noexcept
	{
		Base::etherealize ();
	}

	/// Delete all links to a file or empty directory.
	void remove ();

	///@}

	///@{
	/// File implementation.

	/// File size.
	uint64_t size ();

	///@}

protected:
	File (const DirItemId& id) :
		Base (DirItemId (id))
	{}

private:
	friend class Nirvana::Core::Windows::FileAccess;
	friend class FileAccessDirect;

	void* open (uint32_t access, uint32_t share_mode, uint32_t creation_disposition,
		uint32_t flags_and_attributes);

	uint32_t block_size () const noexcept
	{
		return Base::block_size ();
	}
};

}
}
}

#endif
