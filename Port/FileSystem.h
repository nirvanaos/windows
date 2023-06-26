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
#include <NameService/NamingContextRoot.h>
#include <Nirvana/File.h>
#include "../Windows/Source/WinWChar.h"

namespace Nirvana {
namespace Core {
namespace Port {

class FileSystem
{
public:
	static Roots get_roots ();

	static PortableServer::ServantBase::_ref_type incarnate (const DirItemId& id);

	static void etherealize (const DirItemId& id, CORBA::Object::_ptr_type servant)
	{}

	static Nirvana::DirItem::FileType get_item_type (const DirItemId& id) noexcept
	{
		return (Nirvana::DirItem::FileType)*(const Windows::WinWChar*)id.data ();
	}

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
			if ('\\' == *p) {
				if (p == begin)
					++begin; // Skip adjacent backslashes
				else {
					n.push_back (CosNaming::Core::NamingContextRoot::to_component (
						path.substr (begin - s, p - begin)));
					begin = p + 1;
				}
			} else if (strchr ("<>:\"/\\|?*", *p))
				throw CosNaming::NamingContext::InvalidName ();
		}

		return n;
	}

	// For internal use
	
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
	static DirItemId get_app_data_dir (const IDL::String& name, bool& may_cache);
	static Windows::StringW get_app_data_dir (const Windows::WinWChar* name);
	static DirItemId get_var (const IDL::String&, bool& may_cache);
	static DirItemId get_mnt (const IDL::String&, bool& may_cache);
	static DirItemId get_home (const IDL::String&, bool& may_cache);

	static Windows::StringW make_path (const DirItemId& id)
	{
		return Windows::StringW (id_to_path (id), path_len (id));
	}


	enum class SpecialDir
	{
		var,
		mnt,
		// dev, ...

		END
	};

	static DirItemId make_special_id (SpecialDir dir);

	static SpecialDir is_special_dir (const DirItemId& id) noexcept
	{
		if (id.size () == 4 && get_item_type (id) == Nirvana::DirItem::FileType::directory)
			return (SpecialDir)((const Windows::WinWChar*)id.data ()) [1];
		else
			return SpecialDir::END;
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
