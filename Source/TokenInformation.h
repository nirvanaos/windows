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
#ifndef NIRVANA_CORE_WINDOWS_TOKENINFORMATION_H_
#define NIRVANA_CORE_WINDOWS_TOKENINFORMATION_H_
#pragma once

#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class TokenInformation
{
protected:
	TokenInformation (HANDLE token, TOKEN_INFORMATION_CLASS type);
	
	~TokenInformation ()
	{
		memory->release (buffer_, size_);
	}

	const void* data () const noexcept
	{
		return buffer_;
	}

private:
	void* buffer_;
	size_t size_;
};

template <class T, TOKEN_INFORMATION_CLASS type>
class TokenInfo : private TokenInformation
{
public:
	TokenInfo (HANDLE token) :
		TokenInformation (token, type)
	{}

	const T* operator -> () const noexcept
	{
		return (T*)data ();
	}
};

typedef TokenInfo <TOKEN_USER, ::TokenUser> TokenUser;
typedef TokenInfo <TOKEN_GROUPS, ::TokenGroups> TokenGroups;

}
}
}

#endif
