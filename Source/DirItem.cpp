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

DirItem::DirItem (StringW&& path) :
	handle_ (INVALID_HANDLE_VALUE),
	path_ (std::move (path))
{}

DirItem::DirItem () :
	handle_ (INVALID_HANDLE_VALUE)
{}

DirItem::~DirItem ()
{
	if (INVALID_HANDLE_VALUE != handle_)
		CloseHandle (handle_);
}

void* DirItem::get_handle () const noexcept
{
	if (INVALID_HANDLE_VALUE == handle_) {
		handle_ = CreateFileW (path ().c_str (), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	}
	return handle_;
}

void* DirItem::handle () const
{
	if (INVALID_HANDLE_VALUE == handle_) {
		get_handle ();
		if (INVALID_HANDLE_VALUE == handle_)
			throw_last_error ();
	}
	return handle_;
}

void DirItem::get_attributes (_BY_HANDLE_FILE_INFORMATION& att) const
{
	if (!GetFileInformationByHandle (handle (), &att))
		throw_last_error ();
}

void DirItem::get_file_times (FileTimes& times) const
{
	BY_HANDLE_FILE_INFORMATION att;
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
