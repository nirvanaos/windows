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
#ifndef NIRVANA_CORE_THREADPOSTMAN_H_
#define NIRVANA_CORE_THREADPOSTMAN_H_
#pragma once

#include "Thread.inl"

namespace Nirvana {
namespace Core {
namespace Windows {

class CompletionPort;

/// ThreadPostman class. Thread for thread pool controlled by CompletionPort.
class ThreadPostman :
	public Port::Thread
{
public:
	ThreadPostman (CompletionPort& completion_port) :
		completion_port_ (completion_port)
	{}

protected:
	friend class Thread;
	static unsigned long __stdcall thread_proc (ThreadPostman* _this);

private:
	CompletionPort& completion_port_;
};

}
}
}

#endif
