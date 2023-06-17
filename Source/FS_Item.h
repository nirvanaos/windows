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
#ifndef NIRVANA_CORE_WINDOWS_FS_ITEM_H_
#define NIRVANA_CORE_WINDOWS_FS_ITEM_H_
#pragma once

#include <CORBA/Server.h>
#include <Nirvana/FS.h>
#include "../Port/FileSystem.h"

struct _WIN32_FILE_ATTRIBUTE_DATA;

namespace Nirvana {
namespace FS {
namespace Windows {

// File system item: interface Nirvana::FS::Item
class FS_Item
{
protected:
	FS_Item (const PortableServer::ObjectId& id) :
		id_ (id)
	{}

	const PortableServer::ObjectId& id () const noexcept
	{
		return id_;
	}

	void get_file_times (FileTimes& times) const;

	uint16_t permissions () const
	{
		return 0; // TODO: Implement
	}

	void permissions (uint16_t perms)
	{
		// TODO: Implement
	}

protected:
	typedef Nirvana::Core::Windows::WinWChar WinWChar;

	const WinWChar* path () const noexcept
	{
		return Nirvana::FS::Core::Port::FileSystem::id_to_path (id_);
	}

	void get_attributes (_WIN32_FILE_ATTRIBUTE_DATA& att) const;

private:
	PortableServer::ObjectId id_;
};

}
}
}

#endif
