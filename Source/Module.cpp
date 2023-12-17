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
#include <fnctl.h>
#include <ORB/Services.h>

namespace Nirvana {
namespace Core {

using namespace Windows;

namespace Port {

Module::Module (AccessDirect::_ptr_type file) :
	module_ (nullptr)
{
	StringW tmp_path;
	Dir::_ref_type tmp_dir;
	{
		WinWChar buf [MAX_PATH + 1];
		DWORD cc = GetTempPathW ((DWORD)countof (buf), buf);
		if (!cc)
			throw_UNKNOWN ();
		IDL::String path;
		append_utf8 (buf, buf + cc - 1, path);
		tmp_path.assign (buf, cc);
		CosNaming::Name tmp_dir_name;
		g_system->append_path (tmp_dir_name, path, true);
		auto ns = CosNaming::NamingContextExt::_narrow (CORBA::Core::Services::bind (CORBA::Core::Services::NameService));
		tmp_dir = Dir::_narrow (ns->resolve (tmp_dir_name));
		if (!tmp_dir)
			throw_UNKNOWN ();
	}

	try {
		AccessDirect::_ref_type tmp_file_access;

		{
			IDL::String name (TEMP_MODULE_PREFIX "XXXXXX" TEMP_MODULE_EXT);
			tmp_file_access = AccessDirect::_narrow (tmp_dir->mkostemps (name, 4, O_DIRECT)->_to_object ());
			tmp_path.append (name.begin (), name.end ());
			temp_path_ = tmp_path;
		}

		{
			IDL::Sequence <uint8_t> buf;
			FileLock fl_none;
			file->read (fl_none, 0, std::numeric_limits <uint32_t>::max (), LockType::LOCK_NONE, false, buf);
			tmp_file_access->write (0, buf, fl_none, true);
			tmp_file_access->close ();
		}

		module_ = LoadLibraryW (tmp_path.c_str ());
		if (!module_)
			throw_last_error ();
		Nirvana::Core::PortableExecutable pe (module_);
		if (!pe.find_OLF_section (metadata_))
			throw_BAD_PARAM (make_minor_errno (ENOEXEC));
		const COFF::PE32Header* pehdr = pe.pe32_header ();
		if (!pehdr || pehdr->AddressOfEntryPoint)
			throw_BAD_PARAM (make_minor_errno (ENOEXEC));
	} catch (...) {
		unload ();
		throw;
	}
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
