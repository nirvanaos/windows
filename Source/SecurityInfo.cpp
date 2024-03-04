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
#include "SecurityInfo.h"
#include <AclAPI.h>
#include "error2errno.h"

namespace Nirvana {
namespace Core {
namespace Windows {

SecurityInfo::SecurityInfo (HANDLE handle, SE_OBJECT_TYPE obj_type) :
	psd_ (nullptr)
{
	DWORD err = GetSecurityInfo (handle, obj_type, 
		OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION
		| UNPROTECTED_DACL_SECURITY_INFORMATION,
		&owner_, &group_, &dacl_, nullptr, &psd_);
	if (err)
		throw_win_error_sys (err);
}

ACCESS_MASK SecurityInfo::get_effective_rights (PSID sid, TRUSTEE_TYPE type) const
{
	TRUSTEE_W trustee;
	trustee.pMultipleTrustee = nullptr;
	trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
	trustee.TrusteeForm = TRUSTEE_IS_SID;
	trustee.TrusteeType = type;
	trustee.ptstrName = (LPWCH)sid;

	ACCESS_MASK mask = 0;
	DWORD err = GetEffectiveRightsFromAclW (dacl (), &trustee, &mask);
	if (err)
		throw_win_error_sys (err);

	return mask;
}

}
}
}
