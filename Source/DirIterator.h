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
#ifndef NIRVANA_CORE_WINDOWS_DIRITERATOR_H_
#define NIRVANA_CORE_WINDOWS_DIRITERATOR_H_
#pragma once

#include <NameService/Iterator.h>
#include "win32.h"
#include "WinWChar.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class DirIterator : virtual public CosNaming::Core::Iterator
{
public:
	DirIterator (const WinWChar* pattern);

	~DirIterator ()
	{
		if (INVALID_HANDLE_VALUE != handle_)
			FindClose (handle_);
	}

	virtual bool end () const noexcept override
	{
		return INVALID_HANDLE_VALUE == handle_;
	}

	virtual bool next_one (Binding& b) override;

private:
	bool move_next () noexcept;
	bool false_item () const noexcept;

private:
	WIN32_FIND_DATAW data_;
	HANDLE handle_;
};

}
}
}

#endif
