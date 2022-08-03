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
#ifndef NIRVANA_CORE_PORT_OTHER_DOMAIN_H_
#define NIRVANA_CORE_PORT_OTHER_DOMAIN_H_
#pragma once

#include <ORB/OtherMemory.h>

namespace Nirvana {
namespace ESIOP {

/// Protection domain communication endpoint.
class OtherDomain
{
public:
	~OtherDomain ();
	OtherDomain ();
	OtherDomain (ProtDomainId id);
	OtherDomain (const OtherDomain& src);
	OtherDomain (OtherDomain&& src) NIRVANA_NOEXCEPT;

	OtherDomain& operator = (const OtherDomain& src);
	OtherDomain& operator = (OtherDomain&& src) NIRVANA_NOEXCEPT;

	/// Reset to the null state.
	void reset () NIRVANA_NOEXCEPT;

	/// Send message to domain.
	/// 
	/// \param msg The message.
	/// \param size The message size.
	void send_message (const void* msg, size_t size);

	/// Obtain access to the protection domain memory.
	/// 
	/// \returns OtherMemory interface reference.
	Core::CoreRef <OtherMemory> memory ();

private:
	uint32_t mailslot;
	uint32_t address_space;
};

}
}

#endif
