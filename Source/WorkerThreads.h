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
#ifndef NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_
#define NIRVANA_CORE_WINDOWS_WORKERTHREADS_H_
#pragma once

#include "ThreadWorker.h"
#include "ThreadPool.h"

namespace Nirvana {
namespace Core {

class Startup;

namespace Windows {

template <class Master>
class WorkerThreads :
	public ThreadPool <Master, ThreadWorker>
{
	typedef ThreadPool <Master, ThreadWorker> Pool;
public:
	void run (Startup& startup, DeadlineTime deadline)
	{
		Master::start ();

		// Create other worker threads
		for (ThreadWorker* p = Pool::threads () + 1,
			*end = Pool::threads () + Pool::thread_count (); p != end; ++p) {
			p->create ();
		}

		// Run main for the thread 0 (main thread).
		Pool::threads ()[0].run_main (startup, deadline, Pool::threads () + 1, Pool::thread_count () - 1);
	}

	void shutdown ()
	{
		Master::terminate ();
	}
};

}
}
}

#endif
