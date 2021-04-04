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
#ifndef NIRVANA_CORE_PORT_THREADBACKGROUND_H_
#define NIRVANA_CORE_PORT_THREADBACKGROUND_H_

#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// Platform-specific background thread implementation.
class ThreadBackground :
	public Core::Thread
{
	///@{
	/// Members called from Core.
protected:
	ThreadBackground ();
	~ThreadBackground ();

	/// Create thread
	void create ();

	/// Suspend execution and wait for resume ().
	void suspend ();

	/// Continue execution.
	void resume ();

	/// Temparary boost thread priority above the worker threads priority.
	void priority_boost ();

	/// Restore priority after the priority_boost () call.
	void priority_restore ();
	///@}

private:
	friend class Nirvana::Core::Port::Thread;
	static unsigned long __stdcall thread_proc (ThreadBackground* _this);

private:
	void* event_;
};

}
}
}

#endif
