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

#include <Nirvana/Nirvana.h>
#include <MapUnorderedUnstable.h>
#include <TimerEvent.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// System domain base.
class SysDomain
{
	static const TimeBase::TimeT PROCESS_STARTUP_TIMEOUT = 10 * TimeBase::SECOND;

public:
	uint32_t create_prot_domain (unsigned platform, const IDL::String& host, unsigned port);

	void on_domain_start (uint32_t id) noexcept
	{
		auto f = starting_map_.find (id);
		if (f != starting_map_.end ()) {
			f->second->on_start ();
			starting_map_.erase (f);
		}
	}

private:
	struct StartingProcess
	{
		bool started;
		TimerEvent timer_event;

		StartingProcess () :
			started (false)
		{
			timer_event.set (0, PROCESS_STARTUP_TIMEOUT, 0);
		}

		void on_start () noexcept
		{
			started = true;
			timer_event.cancel ();
			timer_event.signal_event ();
		}
	};

	typedef MapUnorderedUnstable <uint32_t, StartingProcess*, std::hash <uint32_t>, std::equal_to <uint32_t>, UserAllocator> StartingMap;

	StartingMap starting_map_;
};

}
}
}

#endif
