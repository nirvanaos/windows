/*
* Portable Executable file format support library.
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
#ifndef NIRVANA_PE_COFF_H_
#define NIRVANA_PE_COFF_H_
#pragma once

#include <Nirvana/Nirvana.h>
#include "pe_bliss/pe_lib/pe_section.h"

namespace Nirvana {
namespace Core {

class COFF
{
public:
	typedef pe_bliss::pe_win::image_section_header Section;
	typedef pe_bliss::pe_win::image_file_header Header;
	typedef std::conditional <sizeof (void*) == 4,
		pe_bliss::pe_win::image_optional_header32,
		pe_bliss::pe_win::image_optional_header64>::type PE32Header;

	COFF (const void* addr) noexcept :
		hdr_ ((const Header*)addr)
	{}

	const Header* header () const noexcept
	{
		return hdr_;
	}

	const PE32Header* pe32_header () const noexcept;

	const Section* find_section (const char* name) const noexcept;

	const Section* sections () const noexcept;

	uint32_t section_count () const noexcept
	{
		return hdr_->NumberOfSections;
	}

	static bool is_section (const Section& s, const char* name) noexcept;

private:
	const Header* hdr_;

	static const size_t SECTION_NAME_SIZE = 8;
};

}
}

#endif
