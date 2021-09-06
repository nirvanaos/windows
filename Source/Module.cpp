/// Protection domain (process) address space.
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
#include <stdexcept>

using namespace std;

namespace Nirvana {
namespace Core {
namespace Port {

Module::Module (const char* path)
{
	DWORD att = GetFileAttributesA (path);
	if (att & FILE_ATTRIBUTE_DIRECTORY)
		throw runtime_error ("File not found");

	{
		char buf [MAX_PATH + 1];
		if (!GetTempPath (sizeof (buf), buf))
			throw_UNKNOWN ();
		{
			string temp_dir = buf;
			for (UINT uniq = GetTickCount ();; ++uniq) {
				if (!GetTempFileNameA (temp_dir.c_str (), "nex", uniq, buf))
					throw_UNKNOWN ();
				if (!CopyFileA (path, buf, TRUE)) {
					DWORD err = GetLastError ();
					if (ERROR_FILE_EXISTS != err)
						throw_UNKNOWN ();
				} else
					break;
			}
		}
		temp_path_ = buf;
	}
	void* mod = LoadLibraryA (temp_path_.c_str ());
	try {
		if (!mod)
			throw runtime_error ("Can not load module");
		Nirvana::Core::PortableExecutable pe (mod);
		if (!pe.find_OLF_section (metadata_))
			throw runtime_error ("Invalid file format");
		const COFF::PE32Header* pehdr = pe.pe32_header ();
		if (!pehdr || pehdr->AddressOfEntryPoint)
			throw runtime_error ("Invalid file format");
	} catch (...) {
		unload ();
		throw;
	}
	module_ = mod;
}

void Module::call_initialize (ModuleInit::_ptr_type mi)
{
	__try {
		mi->initialize ();
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		throw_UNKNOWN ();
	}
}

void Module::call_terminate (ModuleInit::_ptr_type mi)
{
	__try {
		mi->terminate ();
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		throw_UNKNOWN ();
	}
}

void Module::unload ()
{
	if (module_)
		FreeLibrary (module_);
	if (!temp_path_.empty ())
		DeleteFileA (temp_path_.c_str ());
}

}
}
}
