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
#ifndef NIRVANA_CORE_PORT_WORKER_SEMAPHORE_H_
#define NIRVANA_CORE_PORT_WORKER_SEMAPHORE_H_
#pragma once

#include "../Port/SystemInfo.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerSlave;

class WorkerSemaphore
{
public:
	static unsigned int thread_count ()
	{
		return Port::SystemInfo::hardware_concurrency ();
	}

	void thread_proc (SchedulerSlave& scheduler);

	WorkerSemaphore ()
	{
		handles_ [0] = nullptr;
		verify (handles_ [1] = CreateEventW (nullptr, true, FALSE, nullptr));
	}

	~WorkerSemaphore ()
	{
		for (HANDLE* p = handles_; p != std::end (handles_); ++p) {
			HANDLE h = *p;
			if (h)
				CloseHandle (h);
		}
	}

	void semaphore (HANDLE sem)
	{
		if (handles_ [WORKER_SEMAPHORE])
			CloseHandle (handles_ [WORKER_SEMAPHORE]);
		handles_ [WORKER_SEMAPHORE] = sem;
	}

	HANDLE semaphore ()
	{
		assert (handles_ [WORKER_SEMAPHORE]);
		return handles_ [WORKER_SEMAPHORE];
	}

	void start ()
	{
		assert (handles_ [WORKER_SEMAPHORE]);
		ResetEvent (handles_ [SHUTDOWN_EVENT]);
	}

	void terminate () noexcept
	{
		SetEvent (handles_ [SHUTDOWN_EVENT]);
	}

private:
	enum Handle
	{
		WORKER_SEMAPHORE,
		SHUTDOWN_EVENT,

		HANDLE_COUNT
	};

	HANDLE handles_ [HANDLE_COUNT];
};

}
}
}

#endif
