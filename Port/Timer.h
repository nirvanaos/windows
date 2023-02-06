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
#ifndef NIRVANA_CORE_PORT_TIMER_H_
#define NIRVANA_CORE_PORT_TIMER_H_
#pragma once

#include <CoreInterface.h>
#include <CORBA/TimeBase.h>
#include "../Source/CompletionPortReceiver.h"

typedef void* HANDLE;

namespace Nirvana {
namespace Core {
namespace Port {

class Timer :
	private Windows::CompletionPortReceiver
{
	DECLARE_CORE_INTERFACE

public:
	void set (unsigned flags, TimeBase::TimeT due_time, TimeBase::TimeT period);
	void cancel () NIRVANA_NOEXCEPT;

protected:
	Timer () :
		handle_ (nullptr),
		period_ (0)
	{}

	~Timer ();

	virtual void signal () NIRVANA_NOEXCEPT = 0;

private:
	virtual void completed (_OVERLAPPED* ovl, uint32_t size, uint32_t error) NIRVANA_NOEXCEPT;

private:
	HANDLE handle_;
	long period_;
};

}
}
}

#endif
