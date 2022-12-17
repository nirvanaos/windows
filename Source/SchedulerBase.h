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
#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERBASE_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERBASE_H_
#pragma once

#include "../Port/Scheduler.h"
#include "MessageBroker.h"
#include "AddressSpace.inl"

namespace Nirvana {
namespace Core {
namespace Windows {

class NIRVANA_NOVTABLE SchedulerBase :
	public SchedulerAbstract,
	public Port::Scheduler
{
public:
	SchedulerBase ()
	{
		singleton_ = this;
		other_space_init ();
	}

	~SchedulerBase ()
	{
		singleton_ = nullptr;
	}

	static SchedulerBase& singleton ()
	{
		assert (singleton_);
		//assert (dynamic_cast <Impl*> (singleton_));
		return static_cast <SchedulerBase&> (*singleton_);
	}

	virtual void worker_thread_proc () NIRVANA_NOEXCEPT = 0;

	CompletionPort& completion_port ()
	{
		return message_broker_;
	}

protected:
	HANDLE open_sysdomainid (bool write) NIRVANA_NOEXCEPT;

protected:
	MessageBroker message_broker_;

	static uint32_t sys_process_id_;
};

}
}
}

#endif
