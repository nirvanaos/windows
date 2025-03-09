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
#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERABSTRACT_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERABSTRACT_H_
#pragma once

#include "Executor.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class NIRVANA_NOVTABLE SchedulerAbstract
{
public:
	virtual void create_item () = 0;
	virtual void delete_item () noexcept = 0;
	virtual void schedule (DeadlineTime deadline, Executor& executor) noexcept = 0;
	virtual bool reschedule (DeadlineTime deadline, Executor& executor, DeadlineTime old) noexcept = 0;
	virtual void shutdown () noexcept = 0;
};

}
}
}

#endif
