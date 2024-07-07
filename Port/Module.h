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
#ifndef NIRVANA_CORE_PORT_MODULE_H_
#define NIRVANA_CORE_PORT_MODULE_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include <Nirvana/File.h>
#include <Section.h>
#include <HeapAllocator.h>
#include "../Windows/Source/WinWChar.h"
#include "../pe/PortableExecutable.h"

namespace Nirvana {
namespace Core {
namespace Port {

/// Loadable module
class Module
{
public:
	static uint16_t get_platform (AccessDirect::_ptr_type file)
	{
		return PortableExecutable::get_platform (file);
	}

	void* address () const noexcept
	{
		return module_;
	}

	const Section& metadata () const noexcept
	{
		return metadata_;
	}

	/// Constructor
	/// 
	/// \param file Binary file access.
	Module (AccessDirect::_ptr_type file);

	/// Destructor
	~Module ()
	{
		unload ();
	}

protected:
	Module (const Module&) = delete;
	
	Module (Module&& src) noexcept :
		module_ (src.module_),
		metadata_ (src.metadata_),
		temp_path_ (std::move (src.temp_path_))
	{
		src.module_ = nullptr;
	}

	/// \brief Return all read/write data sections
	/// 
	/// \param sections List of r/w data sections
	void get_data_sections (DataSections& sections) const;

	/// Unload module (only for executables).
	void unload () noexcept;

private:
	typedef std::basic_string <Windows::WinWChar, std::char_traits <Windows::WinWChar>,
		HeapAllocator <Windows::WinWChar> > FilePath;

	void* module_;
	Section metadata_;
	FilePath temp_path_;
};

}
}
}

#endif
