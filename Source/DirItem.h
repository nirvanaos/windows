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

#include "../Port/FileSystem.h"

struct _BY_HANDLE_FILE_INFORMATION;
struct _FILETIME;

namespace Nirvana {
namespace Core {
namespace Windows {

// File system item: interface Nirvana::DirItem
class DirItem
{
public:
	void stat (FileStat& st);

	FileType type () const noexcept
	{
		return type_;
	}

protected:
	const DirItemId& id () const noexcept
	{
		return id_;
	}

	void etherealize () noexcept;

protected:
	DirItem (DirItemId&& id);
	DirItem (FileType type);

	~DirItem ();

	const WinWChar* path () const noexcept
	{
		if (!id_.empty ())
			return Port::FileSystem::id_to_path (id_);
		else
			return nullptr;
	}

	StringW make_path () const
	{
		return Port::FileSystem::make_path (id_);
	}

	size_t path_len () const noexcept
	{
		if (!id_.empty ())
			return Port::FileSystem::path_len (id_);
		else
			return 0;
	}

	bool special () const noexcept
	{
		return id_.empty ();
	}

	void* handle ();

	void* get_handle () noexcept;

	void get_attributes (_BY_HANDLE_FILE_INFORMATION& att);

	enum FileSystemType
	{
		FS_FAT,
		FS_NTFS,
		FS_UNKNOWN
	};

	FileSystemType file_system_type () const noexcept
	{
		return file_system_type_;
	}

	static uint64_t make64 (uint32_t lo, uint32_t hi) noexcept
	{
		return (uint64_t)hi * 0x100000000ui64 + lo;
	}

	static TimeBase::TimeT make_time (const _FILETIME& ft) noexcept;

	static void set_inacc (TimeBase::UtcT& t, uint64_t inac) noexcept;

	uint32_t error_check_exist ();

private:
	const DirItemId id_;
	void* handle_;

protected:
	FileType type_;

private:
	FileSystemType file_system_type_;
	unsigned long file_system_flags_;
	unsigned long max_component_len_;

	typedef uint64_t TimeInaccuracy;

	struct FileSystemTimeInaccuracy
	{
		TimeInaccuracy creation, last_access, last_write;
	};

	struct FileSystemTraits
	{
		const WinWChar* name;
		FileSystemTimeInaccuracy time_inaccuracy;
	};

	static const FileSystemTraits file_systems_ [FS_UNKNOWN];
};

}
}
}

#endif
