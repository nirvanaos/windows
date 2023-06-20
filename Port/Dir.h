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
#ifndef NIRVANA_CORE_PORT_DIR_H_
#define NIRVANA_CORE_PORT_DIR_H_
#pragma once

#include "../Source/DirItem.h"
#include <NameService/NamingContextRoot.h>

namespace Nirvana {
namespace Core {
namespace Port {

class NIRVANA_NOVTABLE Dir : 
	public Windows::DirItem,
	public CosNaming::Core::NamingContextRoot
{
	typedef Windows::DirItem Base;

public:
	void bind_file (CosNaming::Name& n, const DirItemId& file, bool rebind) const
	{
		create_link (n, file, rebind ? FLAG_REBIND : 0);
	}

	void bind_dir (CosNaming::Name& n, const DirItemId& dir, bool rebind) const
	{
		create_link (n, dir, FLAG_DIRECTORY | (rebind ? FLAG_REBIND : 0));
	}

	DirItemId resolve_path (CosNaming::Name& n) const
	{
		return FileSystem::path_to_id (check_path (n, 0).c_str ());
	}

	void unbind (CosNaming::Name& n) const;
	DirItemId create_dir (CosNaming::Name& n) const;
	DirItemId get_new_file_id (CosNaming::Name& n) const;

	virtual std::unique_ptr <CosNaming::Core::Iterator> make_iterator () const override;

protected:
	Dir (const DirItemId& id) :
		Base (id)
	{}

	Windows::StringW check_path (CosNaming::Name& n, size_t rem_cnt) const;
	Windows::StringW create_path (CosNaming::Name& n, size_t rem_cnt, size_t& created_begin) const;

	static Windows::StringW to_wstring (CosNaming::Istring name);
	
	virtual Windows::StringW get_path (CosNaming::Name& n) const;
	Windows::StringW get_path () const;

	static void append_path (Windows::StringW& path, CosNaming::Istring name);

	void remove_created_path (Windows::StringW& path, size_t created_begin) const noexcept;

	static const unsigned FLAG_REBIND = 0x80000000;
	static const unsigned FLAG_DIRECTORY = 0x00000001;
	void create_link (CosNaming::Name& n, const DirItemId& target, unsigned flags) const;

	static void unbind (const Windows::WinWChar* path, uint32_t att);

	Windows::StringW get_pattern () const;
};

}
}
}

#endif