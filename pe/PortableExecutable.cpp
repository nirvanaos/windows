/*
* Nirvana Core.
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
#include "PortableExecutable.h"
#include <Nirvana/throw_exception.h>
#include <Nirvana/OLF.h>

namespace Nirvana {
namespace Core {

void PortableExecutable::check_header (const DOSHeader& hdr)
{
	static const char DOS_magic [] = { 'M', 'Z' };

	if (hdr.e_magic != *(const uint16_t*)DOS_magic)
		throw_BAD_PARAM (make_minor_errno (ENOEXEC));
}

void PortableExecutable::check_header (const NTHeaders32& hdr)
{
	static const char PE_magic [] = { 'P', 'E', '\0', '\0' };

	if (hdr.Signature != *(const uint32_t*)PE_magic)
		throw_BAD_PARAM (make_minor_errno (ENOEXEC));
}

const void* PortableExecutable::get_COFF (const void* base_address)
{
	const DOSHeader* dos_header = (const DOSHeader*)base_address;
	check_header (*dos_header);

	const NTHeaders32* nt_headers = (const NTHeaders32*)((const uint8_t*)base_address + dos_header->e_lfanew);
	check_header (*nt_headers);

	return &nt_headers->FileHeader;
}

PortableExecutable::PortableExecutable (const void* base_address) :
	COFF (get_COFF (base_address)),
	base_address_ (base_address)
{}

const void* PortableExecutable::find_OLF_section (size_t& size) const noexcept
{
	const COFF::Section* ps = COFF::find_section (OLF_BIND);
	if (ps) {
		size = ps->Misc.VirtualSize;
		return section_address (*ps);
	}
	return nullptr;
}

uint_fast16_t PortableExecutable::get_platform (AccessDirect::_ptr_type binary)
{
	Bytes header;

	binary->read (0, sizeof (DOSHeader), header);
	const DOSHeader* dos_header = (const DOSHeader*)header.data ();
	check_header (*dos_header);

	binary->read (dos_header->e_lfanew, sizeof (NTHeaders32), header);
	const NTHeaders32* nt_headers = (const NTHeaders32*)header.data ();
	check_header (*nt_headers);

	return nt_headers->FileHeader.Machine;
}

}
}
