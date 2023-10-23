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
#ifndef NIRVANA_CORE_WINDOWS_OBJECT_NAME_H_
#define NIRVANA_CORE_WINDOWS_OBJECT_NAME_H_
#pragma once

#include "WinWChar.h"

namespace Nirvana {
namespace Core {
namespace Windows {

template <size_t prefix_len>
class ObjectName
{
public:
	ObjectName (const WinWChar* prefix, unsigned id)
	{
		_ultow (id, real_copy (prefix, prefix + prefix_len - 1, name_), 16);
	}

	operator const WinWChar* () const
	{
		return name_;
	}

private:
	WinWChar name_ [prefix_len + 8];
};

template <size_t prefix_len> inline
ObjectName <prefix_len> object_name (const WinWChar (&prefix) [prefix_len], unsigned id)
{
	return ObjectName <prefix_len> (prefix, id);
}

}
}
}

#endif
