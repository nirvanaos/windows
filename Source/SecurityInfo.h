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
#include "TokenInformation.h"
#include "../Port/Security.h"
#include <AclAPI.h>
#include <Nirvana/posix_defs.h>
#include "error2errno.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SecurityInfo
{
public:
	SecurityInfo (HANDLE handle, SE_OBJECT_TYPE obj_type) :
		psd_ (nullptr),
		owner_ (nullptr),
		group_ (nullptr),
		dacl_ (nullptr),
		error_ (0)
	{
		DWORD err = GetSecurityInfo (handle, obj_type,
			OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
			&owner_, &group_, &dacl_, nullptr, &psd_);
		if (err)
			throw_win_error_sys (err);
		else if (IsWellKnownSid (group_, WinNullSid))
			group_ = nullptr;
	}

	SecurityInfo (LPCWSTR name, SE_OBJECT_TYPE obj_type) :
		psd_ (nullptr),
		owner_ (nullptr),
		group_ (nullptr),
		dacl_ (nullptr),
		error_ (0)
	{
		DWORD err = GetNamedSecurityInfoW (name, obj_type,
			OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
			&owner_, &group_, &dacl_, nullptr, &psd_);

		if (err) {
			switch (err) {
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
			case ERROR_ACCESS_DENIED:
				error_ = err;
				break;

			default:
				throw_win_error_sys (err);
			}
		} else if (IsWellKnownSid (group_, WinNullSid))
			group_ = nullptr;
	}

	~SecurityInfo ()
	{
		LocalFree (psd_);
	}

	DWORD error () const noexcept
	{
		return error_;
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

	ACCESS_MASK get_rights (PSID trustee) const noexcept;

private:
	PSECURITY_DESCRIPTOR psd_;
	PSID owner_;
	PSID group_;
	PACL dacl_;
	DWORD error_;
};

class SecurityInfoDirItem : public SecurityInfo
{
public:
	SecurityInfoDirItem (HANDLE handle) :
		SecurityInfo (handle, SE_FILE_OBJECT)
	{}

	SecurityInfoDirItem (LPCWSTR path) :
		SecurityInfo (path, SE_FILE_OBJECT)
	{}

	unsigned get_mode () const
	{
		unsigned mode = 0;

		ACCESS_MASK mask = get_rights (owner ());
		if (mask & FILE_READ_DATA)
			mode |= S_IRUSR;
		if (mask & FILE_WRITE_DATA)
			mode |= S_IWUSR;
		if (mask & FILE_EXECUTE)
			mode |= S_IXUSR;

		if (group ()) {
			mask = get_rights (group ());
			if (mask & FILE_READ_DATA)
				mode |= S_IRGRP;
			if (mask & FILE_WRITE_DATA)
				mode |= S_IWGRP;
			if (mask & FILE_EXECUTE)
				mode |= S_IXGRP;
		}

		mask = get_rights (Port::Security::everyone ());
		if (mask & FILE_READ_DATA)
			mode |= S_IROTH;
		if (mask & FILE_WRITE_DATA)
			mode |= S_IWOTH;
		if (mask & FILE_EXECUTE)
			mode |= S_IXOTH;

		return mode;
	}

	unsigned get_access (const Port::Security::Context& token) const
	{
		TokenUser user (token);

		ACCESS_MASK mask = get_effective_rights (user->User.Sid, TRUSTEE_IS_USER);

		unsigned ret = F_OK;
		if (mask & FILE_READ_DATA)
			ret |= R_OK;
		if (mask & FILE_WRITE_DATA)
			ret |= W_OK;
		if (mask & FILE_EXECUTE)
			ret |= X_OK;
		return ret;
	}

};

}
}
}

#endif
