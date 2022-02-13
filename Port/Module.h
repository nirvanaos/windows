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
#include <Nirvana/ModuleInit.h>
#include <Section.h>
#include <UserAllocator.h>
#include <StringView.h>
#include <forward_list>
#include "../Windows/Source/WinWChar.h"

namespace Nirvana {
namespace Core {
namespace Port {

/// Loadable module
class Module
{
public:
	void* address () const NIRVANA_NOEXCEPT
	{
		return module_;
	}

	const Section& metadata () const NIRVANA_NOEXCEPT
	{
		return metadata_;
	}

protected:
	/// <summary>
	/// 
	/// </summary>
	/// <param name="file"></param>
	Module (const StringView& file);

	~Module ()
	{
		unload ();
	}

	/// \brief Call mi->initialize ();
	///
	/// Implementation must catch all possible failures including the access violation, abort() call etc.
	/// In case of such failures the exception must be thrown.
	static void call_initialize (ModuleInit::_ptr_type mi);

	/// \brief Call mi->terminate ();
	///
	/// Implementation must catch all possible failures including the access violation, abort() call etc.
	/// In case of such failures the exception must be thrown.
	static void call_terminate (ModuleInit::_ptr_type mi);

	/// \brief Return all read/write data sections
	/// 
	/// \param sections List of r/w data sections
	void get_data_sections (std::forward_list <Section, UserAllocator <Section>>& sections);

private:
	void unload ();

private:
	void* module_;
	Section metadata_;
	Nirvana::Core::Windows::SharedStringW temp_path_;
};

}
}
}

#endif
