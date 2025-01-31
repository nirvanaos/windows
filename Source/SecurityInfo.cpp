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

namespace Nirvana {
namespace Core {
namespace Windows {

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

ACCESS_MASK SecurityInfo::get_rights (PSID trustee) const noexcept
{
	ACCESS_MASK mask = 0;
	ACE_HEADER* ace_hdr = (ACE_HEADER*)(dacl_ + 1);
	for (unsigned cnt = dacl_->AceCount; cnt; --cnt) {
		if (!(ace_hdr->AceFlags & INHERIT_ONLY_ACE)) {
			switch (ace_hdr->AceType) {
			case ACCESS_ALLOWED_ACE_TYPE: {
				ACCESS_ALLOWED_ACE* ace = (ACCESS_ALLOWED_ACE*)(ace_hdr);
				if (EqualSid (trustee, &ace->SidStart))
					mask |= ace->Mask;
			} break;
			case ACCESS_DENIED_ACE_TYPE: {
				ACCESS_DENIED_ACE* ace = (ACCESS_DENIED_ACE*)(ace_hdr);
				if (EqualSid (trustee, &ace->SidStart))
					mask &= ~ace->Mask;
			} break;
			}
		}
		ace_hdr = (ACE_HEADER*)((char*)ace_hdr + ace_hdr->AceSize);
	}

	return mask;
}

}
}
}
