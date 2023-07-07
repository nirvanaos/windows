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

#include "../Source/FileSystemImpl.h"
#include <NameService/NamingContextRoot.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// Implements file system operations.
class FileSystem : private Windows::FileSystemImpl
{
	typedef Windows::FileSystemImpl Base;

public:
	/// Obtain root directories
	static Roots get_roots ()
	{
		return Base::get_roots ();
	}

	/// Incarnate file system object.
	/// 
	/// \param id File system object id.
	/// \returns  File system object servant.
	static PortableServer::ServantBase::_ref_type incarnate (const DirItemId& id)
	{
		return Base::incarnate (id);
	}

	/// Etherealize file system object.
	/// 
	/// \param id      File system object id.
	/// \param servant File system object servant.
	static void etherealize (const DirItemId& id, CORBA::Object::_ptr_type servant)
	{
		Base::etherealize (id, servant);
	}

	/// Get item type by id.
	/// 
	/// \param id File system object id.
	/// \returns File system object type.
	static Nirvana::FileType get_item_type (const DirItemId& id) noexcept
	{
		return Base::get_item_type (id);
	}

	/// Get Naming Service name from file system path.
	/// 
	/// \param path File or directory path.
	/// \returns Naming Service compound name.
	static CosNaming::Name get_name_from_path (const IDL::String& path)
	{
		CosNaming::Name n;
		const char* s = path.c_str ();
		const char* begin = s;
		if (path.size () > 3 && begin [1] == ':' && begin [2] == '\\') {

			char drive = *begin;
			if ('a' <= drive && drive <= 'z')
				drive += 'A' - 'a';
			else if (!('A' <= drive && drive <= 'Z'))
				throw CosNaming::NamingContext::InvalidName ();

			n.push_back (CosNaming::NameComponent ("/", IDL::String ()));
			n.push_back (CosNaming::NameComponent ("mnt", IDL::String ()));

			IDL::String drv;
			drv += drive;
			drv += ':';
			n.push_back (CosNaming::NameComponent (drv, IDL::String ()));
			begin += 3;
		}
		
		for (const char* p = begin; *p; ++p) {
			if ('\\' == *p || '/' == *p) {
				if (p == begin)
					++begin; // Skip adjacent backslashes
				else {
					n.push_back (CosNaming::Core::NamingContextRoot::to_component (
						path.substr (begin - s, p - begin)));
					begin = p + 1;
				}
			} else if (strchr ("<>:\"|?*", *p))
				throw CosNaming::NamingContext::InvalidName ();
		}
		if (*begin) // Ignore trailing slash
			n.push_back (CosNaming::Core::NamingContextRoot::to_component (path.substr (begin - s)));

		return n;
	}
};

}
}
}

#endif
