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
#ifndef NIRVANA_CORE_WINDOWS_DIRITEM_H_
#define NIRVANA_CORE_WINDOWS_DIRITEM_H_
#pragma once

#include "WinWChar.h"

struct _WIN32_FILE_ATTRIBUTE_DATA;

namespace Nirvana {
namespace Core {
namespace Windows {

// File system item: interface Nirvana::DirItem
class DirItem
{
public:
	void get_file_times (FileTimes& times) const;

	uint16_t permissions () const
	{
		return 0; // TODO: Implement
	}

	void permissions (uint16_t perms)
	{
		// TODO: Implement
	}

	const StringW& path () const noexcept
	{
		return path_;
	}

protected:
	DirItem (StringW&& path);

	DirItem ()
	{}

	void get_attributes (_WIN32_FILE_ATTRIBUTE_DATA& att) const;

private:
	const StringW path_;
};

}
}
}

#endif
