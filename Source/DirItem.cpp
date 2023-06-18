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
#include "DirItem.h"
#include "win32.h"
#include "error2errno.h"

namespace Nirvana {
namespace Core {
namespace Windows {

void DirItem::get_attributes (_WIN32_FILE_ATTRIBUTE_DATA& att) const
{
	if (!GetFileAttributesExW (path (), GetFileExInfoStandard, &att))
		throw RuntimeError (error2errno (GetLastError ()));
}

void DirItem::get_file_times (FileTimes& times) const
{
	WIN32_FILE_ATTRIBUTE_DATA att;
	get_attributes (att);

	// TODO: Calculate time inacuracy based on the underlying file fystem type.
	ULARGE_INTEGER ui;

	ui.LowPart = att.ftCreationTime.dwLowDateTime;
	ui.HighPart = att.ftCreationTime.dwHighDateTime;
	times.creation_time (TimeBase::UtcT (ui.QuadPart + WIN_TIME_OFFSET_SEC * TimeBase::SECOND, 0, 0, 0));
	
	ui.LowPart = att.ftLastAccessTime.dwLowDateTime;
	ui.HighPart = att.ftLastAccessTime.dwHighDateTime;
	times.last_access_time (TimeBase::UtcT (ui.QuadPart + WIN_TIME_OFFSET_SEC * TimeBase::SECOND, 0, 0, 0));

	ui.LowPart = att.ftLastWriteTime.dwLowDateTime;
	ui.HighPart = att.ftLastWriteTime.dwHighDateTime;
	times.last_write_time (TimeBase::UtcT (ui.QuadPart + WIN_TIME_OFFSET_SEC * TimeBase::SECOND, 0, 0, 0));
}

}
}
}
