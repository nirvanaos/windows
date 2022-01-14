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
#ifndef NIRVANA_CORE_WINDOWS_MAILSLOT_H_
#define NIRVANA_CORE_WINDOWS_MAILSLOT_H_
#pragma once

#include "WinWChar.h"
#include <stdlib.h>

namespace Nirvana {
namespace Core {
namespace Windows {

/// Mailslot class.
class Mailslot
{
public:
	Mailslot () :
		mailslot_ ((void*)(intptr_t)-1)
	{}

	Mailslot (Mailslot&& src) :
		mailslot_ (src.mailslot_)
	{
		src.mailslot_ = nullptr;
	}

	~Mailslot ()
	{
		close ();
	}

	bool open (const WinWChar* name);

	template <typename Msg>
	void send (const Msg& msg)
	{
		send (&msg, sizeof (msg));
	}

	void send (const void* msg, uint32_t size);

	void close ();

	bool is_open () const
	{
		return mailslot_ != (void*)(intptr_t)-1;
	}

private:
	void* mailslot_;
};

}
}
}

#endif
