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
#ifndef NIRVANA_CORE_PORT_BACKOFF_H_
#define NIRVANA_CORE_PORT_BACKOFF_H_

#include <Nirvana/Nirvana.h>

extern "C" __declspec (dllimport)
void __stdcall Sleep (unsigned long dwMilliseconds);

extern "C" __declspec (dllimport)
int __stdcall SwitchToThread (void);

namespace Nirvana {
namespace Core {
namespace Port {

class BackOff
{
protected:
	/// A number of iterations when we should call yield ().
	/// SwitchToThread () experiences the expensive cost of a context switch, which can be 10000+ cycles.
	/// It also suffers the cost of ring 3 to ring 0 transitions, which can be 1000+ cycles.
	static const unsigned ITERATIONS_YIELD = 11000;

	/// Maximal number of iterations.
	static const unsigned ITERATIONS_MAX = 1000000;

	static void yield (unsigned iterations)
	{
		::SwitchToThread ();
	}
};

}
}
}

#endif
