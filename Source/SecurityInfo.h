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
#ifndef NIRVANA_CORE_WINDOWS_SECURITYINFO_H_
#define NIRVANA_CORE_WINDOWS_SECURITYINFO_H_
#pragma once

#include "win32.h"
#include <AclAPI.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class SecurityInfo
{
public:
	SecurityInfo (HANDLE handle, SE_OBJECT_TYPE obj_type);

	~SecurityInfo ()
	{
		LocalFree (psd_);
	}

	PSID owner () const noexcept
	{
		return owner_;
	}

	PSID group () const noexcept
	{
		return group_;
	}

	PACL dacl () const noexcept
	{
		return dacl_;
	}

	ACCESS_MASK get_effective_rights (PSID sid, TRUSTEE_TYPE type) const;

private:
	PSECURITY_DESCRIPTOR psd_;
	PSID owner_;
	PSID group_;
	PACL dacl_;
};

}
}
}

#endif
