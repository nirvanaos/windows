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
#pragma once

#include <Thread.h>

namespace Nirvana {
namespace Core {
namespace Port {

/// Platform-specific background thread implementation.
class NIRVANA_NOVTABLE ThreadBackground :
	public Core::Thread
{
	///@{
	/// Members called from Core.
public:

	/// Suspend execution and wait for resume ().
	virtual void yield () NIRVANA_NOEXCEPT;

	/// Continue execution.
	void resume () NIRVANA_NOEXCEPT;

protected:
	ThreadBackground ();
	~ThreadBackground ();

	/// Create thread
	void start ();

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
