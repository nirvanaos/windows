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
#ifndef NIRVANA_CORE_WINDOWS_SYSDOMAINID_H_
#define NIRVANA_CORE_WINDOWS_SYSDOMAINID_H_
#pragma once

#include "win32.h"

typedef void* HANDLE;

namespace Nirvana {
namespace Core {
namespace Windows {

extern uint32_t sys_process_id;

HANDLE open_sysdomainid (bool write);
bool get_sys_process_id ();

// Returns application data root folder, with trailing slash.
long get_app_data_path (WCHAR* path, bool create) noexcept;

// Returns application data folder, without trailing slash.
long get_app_data_folder (const WCHAR* folder, WCHAR* path, bool create) noexcept;

}
}
}

#endif
