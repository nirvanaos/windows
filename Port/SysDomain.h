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
#ifndef NIRVANA_CORE_PORT_SYSDOMAIN_H_
#define NIRVANA_CORE_PORT_SYSDOMAIN_H_
#pragma once

#include "../Source/Mailslot.h"

namespace Nirvana {
namespace Core {
namespace Port {

class SysDomain
{
public:
	class ProtDomainInfo
	{
	public:
		struct ProcessStart;
		ProtDomainInfo (int platform); // Will invoke CreateProcess().
		ProtDomainInfo (const ProcessStart& message);

		ProtDomainInfo (ProtDomainInfo&& src) :
			process_id_ (src.process_id_),
			process_ (src.process_),
			semaphore_ (src.semaphore_),
			mailslot_ (std::move (src.mailslot_))
		{
			src.process_ = nullptr;
			src.semaphore_ = nullptr;
		}

		void process_start ();

		~ProtDomainInfo ();

		uint32_t domain_id () const
		{
			return process_id_;
		}

	private:
		void create_semaphore ();

	private:
		uint32_t process_id_;
		void* process_;
		void* semaphore_;
		Windows::Mailslot mailslot_;
	};
};

}
}
}

#endif
