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
#ifndef NIRVANA_CORE_WINDOWS_EXECCONTEXT_INL_
#define NIRVANA_CORE_WINDOWS_EXECCONTEXT_INL_
#pragma once

#include "../Port/ExecContext.h"
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

inline
bool ExecContext::initialize () noexcept
{
#ifndef NDEBUG
	dbg_main_thread_id_ = GetCurrentThreadId ();
#endif
	if (FLS_OUT_OF_INDEXES == (current_ = FlsAlloc (nullptr)))
		return false;
	main_fiber_ = ConvertThreadToFiberEx (nullptr, FIBER_FLAG_FLOAT_SWITCH);
	return main_fiber_ != nullptr;
}

inline
void ExecContext::terminate () noexcept
{
	assert (dbg_main_thread_id_ == GetCurrentThreadId ());
	ConvertFiberToThread ();
	FlsFree (current_);
}

inline
void ExecContext::current (Core::ExecContext* context) noexcept
{
	FlsSetValue (current_, context);
}

inline
void ExecContext::convert_to_thread () noexcept
{
	// Static function, thread object may be already destructed here.
	verify (ConvertFiberToThread ());
}

}
}
}

#endif
