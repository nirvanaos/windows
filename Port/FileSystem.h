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
#include <array>

namespace Nirvana {
namespace Core {
namespace Port {

/// Implements file system operations.
class FileSystem : private Windows::FileSystemImpl
{
	typedef Windows::FileSystemImpl Base;

public:
	/// Obtain root directories
	static Roots get_roots ();

	/// Incarnate file system object.
	/// 
	/// \param id File system object id.
	/// \returns  File system object servant.
	static PortableServer::ServantBase::_ref_type incarnate (const DirItemId& id);

	/// Etherealize file system object.
	/// 
	/// \param id      File system object id.
	/// \param servant File system object servant.
	static void etherealize (const DirItemId& id, CORBA::Object::_ptr_type servant)
	{} // Do nothing

	/// Get item type by id.
	/// 
	/// \param id File system object id.
	/// \returns File system object type.
	static Nirvana::FileType get_item_type (const DirItemId& id) noexcept
	{
		return Base::get_item_type (id);
	}

	/// Translate path from a host-specific form to standard.
	/// 
	/// On the UNIX-like systems probably does nothing and just returns `false`.
	/// 
	/// \param path Host-specific path.
	/// \param [out] translated Translated standard path.
	/// \returns `true` if path was translated and \p translated string is not empty.
	static bool translate_path (const IDL::String& path, IDL::String& translated);

	/// Line endings sequence.
	/// For UNIX \n line endings must return {0, 0}.
	static std::array <char, 2> eol () noexcept
	{
		return { '\r', '\n' };
	}

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
