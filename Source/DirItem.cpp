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

const DirItem::FileSystemTraits DirItem::file_systems_ [FS_UNKNOWN] = {
	{ WINWCS ("FAT"), { 10 * TimeBase::MILLISECOND, 1 * TimeBase::DAY, 2 * TimeBase::SECOND } },
	{ WINWCS ("NTFS"), { 10 * TimeBase::MILLISECOND, 1 * TimeBase::HOUR, 2 * TimeBase::SECOND } }
};

DirItem::DirItem (StringW&& path) :
	path_ (std::move (path)),
	handle_ (INVALID_HANDLE_VALUE),
	file_system_type_ (FS_UNKNOWN),
	file_system_flags_ (0),
	max_component_len_ (0)
{}

DirItem::DirItem () :
	handle_ (INVALID_HANDLE_VALUE),
	file_system_type_ (FS_UNKNOWN),
	file_system_flags_ (0),
	max_component_len_ (0)
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

		if (INVALID_HANDLE_VALUE != handle_) {
			WinWChar buf [MAX_PATH + 1];
			if (GetVolumeInformationByHandleW (handle_, nullptr, 0, nullptr, &max_component_len_, &file_system_flags_,
				buf, (DWORD)std::size (buf))
				) {
				for (const FileSystemTraits* pfs = file_systems_; pfs != std::end (file_systems_); ++pfs) {
					if (!wcscmp (pfs->name, buf)) {
						file_system_type_ = (FileSystemType)(pfs - file_systems_);
						break;
					}
				}
			}
		}
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

	if (file_system_type_ < FS_UNKNOWN) {
		const FileSystemTraits& fst = file_systems_ [file_system_type_];
		times.creation_time ().inacchi ((uint32_t)fst.time_inaccuracy.creation);
		times.creation_time ().inacclo ((uint16_t)(fst.time_inaccuracy.creation >> 32));
		times.last_access_time ().inacchi ((uint32_t)fst.time_inaccuracy.last_access);
		times.last_access_time ().inacclo ((uint16_t)(fst.time_inaccuracy.last_access >> 32));
		times.last_write_time ().inacchi ((uint32_t)fst.time_inaccuracy.last_write);
		times.last_write_time ().inacclo ((uint16_t)(fst.time_inaccuracy.last_write >> 32));
	}
}

}
}
}
