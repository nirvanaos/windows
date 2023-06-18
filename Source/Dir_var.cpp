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
#include "Dir_var.h"
#include "win32.h"
#include "error2errno.h"
#include "DirIteratorEx.h"

namespace Nirvana {
namespace Core {
namespace Windows {

StringW Dir_var::get_path (CosNaming::Name& n) const
{
	assert (!n.empty ());
	if (n.front ().id () == "tmp" && n.front ().kind ().empty ()) {
		n.erase (n.begin ());
		WinWChar buf [MAX_PATH + 1];
		DWORD cc = GetTempPathW ((DWORD)std::size (buf), buf);
		if (!cc)
			throw_last_error ();

		return Windows::StringW (buf, cc - 1);
	} else
		return Base::get_path ();
}

std::unique_ptr <CosNaming::Core::Iterator> Dir_var::make_iterator () const
{
	std::unique_ptr <DirIteratorEx> iter (std::make_unique <DirIteratorEx> (get_pattern ().c_str ()));
	iter->push ("tmp");
	return iter;
}

}
}
}
