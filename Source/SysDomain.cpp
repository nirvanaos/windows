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
#include "../Port/SysDomain.h"
#include "../Port/SystemInfo.h"
#include "SchedulerMaster.h"
#include "Message.h"
#include "MailslotName.h"

namespace Nirvana {
namespace Core {
namespace Port {

struct SysDomain::ProtDomainInfo::ProcessStart : Windows::Message::ProcessStart
{};

SysDomain::ProtDomainInfo::ProtDomainInfo (int platform) :
	process_ (nullptr),
	semaphore_ (nullptr)
{
	create_semaphore ();
	// TODO: Call CreateProcess with parameters "-p <sysdom process id> <semaphore handle>".
	// Initialize process_id_ and process_ with returned values.
}

SysDomain::ProtDomainInfo::ProtDomainInfo (const ProcessStart& message) :
	process_id_ (message.process_id),
	process_ (nullptr),
	semaphore_ (nullptr)
{
}

SysDomain::ProtDomainInfo::~ProtDomainInfo ()
{
	if (process_)
		CloseHandle (process_);
	if (semaphore_) {
		// Release unused processor cores
		while (WAIT_OBJECT_0 == WaitForSingleObject (semaphore_, 0))
			Windows::SchedulerMaster::singleton ().core_free ();
		CloseHandle (semaphore_);
	}
}

void SysDomain::ProtDomainInfo::create_semaphore ()
{
	if (!(semaphore_ = CreateSemaphoreW (nullptr, 0, (LONG)SystemInfo::hardware_concurrency (), nullptr)))
		throw_NO_MEMORY ();
}

void SysDomain::ProtDomainInfo::process_start ()
{
	mailslot_.open (Windows::MailslotName (process_id_));
	if (!process_) {
		try {
			if (!(process_ = OpenProcess (SYNCHRONIZE | PROCESS_TERMINATE, FALSE, process_id_)))
				throw_INTERNAL ();
			create_semaphore ();
		} catch (...) {
			try {
				mailslot_.send (Windows::Message::ProcessStartResponse (0, 0));
			} catch (...) {
			}
		}
		mailslot_.send (Windows::Message::ProcessStartResponse (GetCurrentProcessId (), semaphore_));
	}
}

}
}
}
