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
#ifndef NIRVANA_CORE_PORT_FILESYSTEM_H_
#define NIRVANA_CORE_PORT_FILESYSTEM_H_
#pragma once

#include <NameService/Roots.h>
#include <Nirvana/File.h>
#include "../Windows/Source/WinWChar.h"

namespace Nirvana {
namespace Core {

namespace Windows {
class DirItem;
}

namespace Port {

class FileSystem
{
public:
	static Roots get_roots ();

	static PortableServer::ServantBase::_ref_type incarnate (const DirItemId& id);

	static void etherealize (const DirItemId& id, PortableServer::ServantBase::_ptr_type servant)
	{}

	static Nirvana::DirItem::FileType get_item_type (const DirItemId& id) noexcept
	{
		return (Nirvana::DirItem::FileType)*(const Windows::WinWChar*)id.data ();
	}
	
	static DirItemId path_to_id (const Windows::WinWChar* path, Nirvana::DirItem::FileType type =
		Nirvana::DirItem::FileType::unknown);

	static const Windows::WinWChar* id_to_path (const DirItemId& id) noexcept
	{
		return ((const Windows::WinWChar*)id.data ()) + 1;
	}

	static size_t path_len (const DirItemId& id) noexcept
	{
		return id.size () / 2 - 2;
	}

private:
	static DirItemId get_var (const IDL::String&, bool& may_cache);

private:
	struct Root
	{
		const char* dir;
		GetRootId factory;
	};
	
	static const Root roots_ [];
};

}
}
}

#endif
