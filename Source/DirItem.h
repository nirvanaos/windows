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

struct _BY_HANDLE_FILE_INFORMATION;

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
		assert (!path_.empty ());
		return path_;
	}

protected:
	DirItem (StringW&& path);
	DirItem ();

	~DirItem ();

	void* handle () const;

	void* get_handle () const noexcept;

	void get_attributes (_BY_HANDLE_FILE_INFORMATION& att) const;

protected:
	mutable void* handle_;

private:
	const StringW path_;
};

}
}
}

#endif
