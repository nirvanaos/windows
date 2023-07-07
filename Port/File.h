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
namespace Port {

class File : public Windows::DirItem
{
	typedef Windows::DirItem Base;

protected:
	File (const DirItemId& id) :
		Base (DirItemId (id))
	{}

	uint64_t size ();

	FileType type () noexcept;

	void remove ();

public:
	void* open (uint32_t access, uint32_t share_mode, uint32_t creation_disposition,
		uint32_t flags_and_attributes);

};

}
}
}

#endif
