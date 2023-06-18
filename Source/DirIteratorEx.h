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
#ifndef NIRVANA_CORE_WINDOWS_DIRITERATOREX_H_
#define NIRVANA_CORE_WINDOWS_DIRITERATOREX_H_
#pragma once

#include "DirIterator.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class DirIteratorEx : public DirIterator
{
	typedef DirIterator Base;

public:
	DirIteratorEx (const WinWChar* pattern) :
		Base (pattern)
	{}

	void reserve (size_t cnt)
	{
		stack_.reserve (cnt);
	}

	void push (const char* name, CosNaming::BindingType type = CosNaming::BindingType::ncontext);

	virtual bool end () const noexcept override
	{
		return Base::end () && stack_.empty ();
	}

	virtual bool next_one (Binding& b) override;

private:
	std::vector <Binding> stack_;
};

}
}
}

#endif
