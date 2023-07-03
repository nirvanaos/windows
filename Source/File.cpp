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
#include "../Port/File.h"
#include "win32.h"
#include "error2errno.h"

namespace Nirvana {
namespace Core {

using namespace Windows;

namespace Port {

uint64_t File::size () const
{
	BY_HANDLE_FILE_INFORMATION att;
	get_attributes (att);
	ULARGE_INTEGER ui;

	ui.LowPart = att.nFileSizeLow;
	ui.HighPart = att.nFileSizeHigh;

	return ui.QuadPart;
}

Nirvana::FileType File::type () const noexcept
{
	if (Nirvana::FileType::none == type_) {
		HANDLE h = get_handle ();
		if (INVALID_HANDLE_VALUE == h)
			type_ = Nirvana::FileType::not_found;
		else {
			switch (GetFileType (h)) {
			case FILE_TYPE_CHAR:
				type_ = Nirvana::FileType::character;
				break;

			case FILE_TYPE_DISK:
				type_ = Nirvana::FileType::regular;
				break;

			case FILE_TYPE_PIPE:
				type_ = Nirvana::FileType::fifo;
				break;

			default:
				type_ = Nirvana::FileType::unknown;
			}
		}
	}

	return type_;
}

void* File::open (uint32_t access, uint32_t share_mode, uint32_t creation_disposition,
	uint32_t flags_and_attributes) const
{
	HANDLE h = CreateFileW (path ().c_str (),
		access, share_mode, nullptr, creation_disposition, flags_and_attributes, nullptr);
/*
	if (INVALID_HANDLE_VALUE != h && INVALID_HANDLE_VALUE == handle_) {
		HANDLE cp = GetCurrentProcess ();
		DuplicateHandle (cp, h, cp, &handle_, FILE_READ_ATTRIBUTES, false, 0);
	}
*/
	return h;
}

}
}
}
