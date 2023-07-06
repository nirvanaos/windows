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
#include "../Port/Module.h"
#include <PortableExecutable.h>
#include "win32.h"
#include "error2errno.h"
#include <Nirvana/string_conv.h>

namespace Nirvana {
namespace Core {

using namespace Windows;

namespace Port {

Module::Module (const StringView& file)
{
	WCHAR temp_path [MAX_PATH + 1];

	{
		SharedStringW wpath;
		utf8_to_wide (file, wpath);
		DWORD att = GetFileAttributesW (wpath.c_str ());
		if (att & FILE_ATTRIBUTE_DIRECTORY)
			throw RuntimeError (ENOENT);
		WCHAR temp_dir [MAX_PATH + 1];
		if (!GetTempPathW ((DWORD)countof(temp_dir), temp_dir))
			throw_UNKNOWN ();
		for (UINT uniq = GetTickCount ();; ++uniq) {
			if (!GetTempFileNameW (temp_dir, WINWCS("nex"), uniq, temp_path))
				throw_UNKNOWN ();
			if (!CopyFileW (wpath.c_str (), temp_path, TRUE)) {
				DWORD err = GetLastError ();
				if (ERROR_FILE_EXISTS != err)
					throw_UNKNOWN ();
			} else
				break;
		}
	}
	temp_path_ = temp_path;
	void* mod = LoadLibraryW (temp_path_.c_str ());
	try {
		if (!mod)
			throw_last_error ();
		Nirvana::Core::PortableExecutable pe (mod);
		if (!pe.find_OLF_section (metadata_))
			throw RuntimeError (ENOEXEC);
		const COFF::PE32Header* pehdr = pe.pe32_header ();
		if (!pehdr || pehdr->AddressOfEntryPoint)
			throw RuntimeError (ENOEXEC);
	} catch (...) {
		unload ();
		throw;
	}
	module_ = mod;
}

void Module::unload ()
{
	if (module_)
		FreeLibrary (module_);
	if (!temp_path_.empty ())
		DeleteFileW (temp_path_.c_str ());
}

void Module::get_data_sections (DataSections& sections)
{
	Nirvana::Core::PortableExecutable pe (module_);
	for (const COFF::Section* s = pe.sections (), *end = s + pe.section_count (); s != end; ++s) {
		if (s->Characteristics & IMAGE_SCN_MEM_WRITE
			&& !COFF::is_section (*s, ".msvcjmc")
			&& !COFF::is_section (*s, OLF_BIND)
			)
			sections.push_front ({ pe.section_address (*s), s->VirtualSize });
	}
}

}
}
}
