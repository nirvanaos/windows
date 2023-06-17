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

#include <CORBA/Server.h>
#include <Nirvana/FS_s.h>
#include <NameService/FileSystemRoots.h>
#include "../Windows/Source/WinWChar.h"

namespace Nirvana {
namespace FS {
namespace Core {
namespace Port {

class FileSystem
{
public:
	static Roots get_roots ();
	static PortableServer::ObjectId get_unique_id (const CosNaming::Name& name);

	static CosNaming::BindingType get_binding_type (const PortableServer::ObjectId& id)
	{
		return (CosNaming::BindingType)*(const uint16_t*)id.data ();
	}
	
	static PortableServer::ServantBase::_ref_type incarnate (const PortableServer::ObjectId& id);

	static void etherealize (const PortableServer::ObjectId& id,
		PortableServer::ServantBase::_ptr_type servant)
	{}

private:
	static PortableServer::ObjectId get_var (const IDL::String&, bool& may_cache);

	static PortableServer::ObjectId path_to_id (CosNaming::BindingType type,
		const Nirvana::Core::Windows::WinWChar* path, size_t len);

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
}

#endif
