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

void Module::load (const char* path)
{
	void* mod = LoadLibraryA (path);
	if (!mod)
		throw runtime_error ("File not found");
	try {
		Nirvana::Core::PortableExecutable pe (mod);
		if (!pe.find_OLF_section (metadata_))
			throw runtime_error ("Invalid file format");
		if (pe.header ()->SizeOfOptionalHeader >= sizeof (llvm::COFF::PE32Header)) {
			const llvm::COFF::PE32Header* pehdr = (const llvm::COFF::PE32Header*)(pe.header () + 1);
			if (
				(pehdr->Magic != llvm::COFF::PE32Header::PE32 && pehdr->Magic != llvm::COFF::PE32Header::PE32_PLUS)
				|| pehdr->AddressOfEntryPoint
				)
				throw runtime_error ("Invalid file format");
		} else
			throw runtime_error ("Invalid file format");
	} catch (...) {
		FreeLibrary (mod);
		throw;
	}
	module_ = mod;
}

void Module::unload ()
{
	if (module_) {
		FreeLibrary (module_);
		module_ = nullptr;
	}
}

}
}
}
