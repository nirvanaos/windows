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
#include "../Port/FileSystem.h"
#include "app_data.h"
#include <algorithm>

namespace Nirvana {

using namespace Core::Windows;

namespace FS {
namespace Core {
namespace Port {

const FileSystem::Root FileSystem::roots_ [] = {
	{ "var", get_var }
};

Roots FileSystem::get_roots ()
{
	Roots roots;
	for (const Root* p = roots_; p != std::end (roots_); ++p) {
		roots.push_back ({ p->dir, p->factory });
	}
	return roots;
}

PortableServer::ObjectId FileSystem::get_var (const IDL::String&, bool& may_cache)
{
	WCHAR path [MAX_PATH + 1];
	size_t cc = get_app_data_folder (path, std::size (path), WINWCS ("var"), true);
	may_cache = true;
	return path_to_id (CosNaming::BindingType::ncontext, path, cc);
}

PortableServer::ObjectId FileSystem::path_to_id (CosNaming::BindingType type, const WinWChar* path, size_t len)
{
	PortableServer::ObjectId id;
	id.resize (len + 2 * sizeof (WCHAR));
	WCHAR* p = (WCHAR*)id.data ();
	*(p++) = (WCHAR)type;
	std::copy (path, path + len + 1, p);
	return id;
}

PortableServer::ServantBase::_ref_type FileSystem::incarnate (const PortableServer::ObjectId& id)
{
	throw CORBA::NO_IMPLEMENT ();
}

}
}
}
}
