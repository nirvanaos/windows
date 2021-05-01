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
#ifndef NIRVANA_CORE_PORT_MODULE_H_
#define NIRVANA_CORE_PORT_MODULE_H_

#include <Nirvana/Nirvana.h>

extern int __stdcall FreeLibrary (void* mod);

namespace Nirvana {
namespace Core {
namespace Port {

class Module
{
public:
	static Module* load (const std::string& file)
	{
		return reinterpret_cast <Module*> (LoadLibraryA (file.c_str ()));
	}

	void* address () const NIRVANA_NOEXCEPT
	{
		return const_cast <Module*> (this);
	}

	~Module ()
	{
		FreeLibrary (this);
	}

	void operator delete (void*) NIRVANA_NOEXCEPT {};
};

}
}
}

#endif
