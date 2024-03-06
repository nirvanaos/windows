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
#include "FileSecurityAttributes.h"
#include "error2errno.h"
#include <Nirvana/posix.h>
#include <AccCtrl.h>
#include <AclAPI.h>

namespace Nirvana {
namespace Core {
namespace Windows {

FileSecurityAttributes::FileSecurityAttributes () :
	psa_ (nullptr)
{
	zero (sa_);
}

void FileSecurityAttributes::initialize (unsigned mode, bool dir)
{
	if (mode) {

		if (!(mode & S_IRWXU))
			mode |= S_IRWXU;

		DWORD inheritance = dir ? SUB_CONTAINERS_AND_OBJECTS_INHERIT : NO_INHERITANCE;

		EXPLICIT_ACCESS_W ea [3];
		zero (ea);

		ea [0].grfAccessMode = SET_ACCESS;
		ea [0].grfInheritance = inheritance;
		ea [0].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea [0].Trustee.ptstrName = (LPWCH)Port::Security::creator_owner ();
		if (mode & S_IRUSR)
			ea [0].grfAccessPermissions |= FILE_READ_DATA;
		if (mode & S_IWUSR)
			ea [0].grfAccessPermissions |= FILE_WRITE_DATA;
		if (mode & S_IXUSR)
			ea [0].grfAccessPermissions |= FILE_EXECUTE;
		EXPLICIT_ACCESS_W* ea_end = ea + 1;

//		if (mode & S_IRWXG) {
			ea_end->grfAccessMode = SET_ACCESS;
			ea_end->grfInheritance = inheritance;
			ea_end->Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			ea_end->Trustee.ptstrName = (LPWCH)Port::Security::creator_group ();
			if (mode & S_IRGRP)
				ea_end->grfAccessPermissions |= FILE_READ_DATA;
			if (mode & S_IWGRP)
				ea_end->grfAccessPermissions |= FILE_WRITE_DATA;
			if (mode & S_IXGRP)
				ea_end->grfAccessPermissions |= FILE_EXECUTE;
			++ea_end;
//		}

		if (mode & S_IRWXO) {
			ea_end->grfAccessMode = SET_ACCESS;
			ea_end->grfInheritance = inheritance;
			ea_end->Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			ea_end->Trustee.ptstrName = (LPWCH)Port::Security::everyone ();
			if (mode & S_IROTH)
				ea_end->grfAccessPermissions |= FILE_READ_DATA;
			if (mode & S_IWOTH)
				ea_end->grfAccessPermissions |= FILE_WRITE_DATA;
			if (mode & S_IXOTH)
				ea_end->grfAccessPermissions |= FILE_EXECUTE;
			++ea_end;
		}

		// Create a new ACL that contains the new ACEs.
		PACL acl = nullptr;
		DWORD err = SetEntriesInAclW ((ULONG)(ea_end - ea), ea, nullptr, &acl);
		if (err)
			Windows::throw_win_error_sys (err);

		try {

			// Initialize a security descriptor.
			size_t cb = SECURITY_DESCRIPTOR_MIN_LENGTH;
			sa_.lpSecurityDescriptor = (SECURITY_DESCRIPTOR*)memory->allocate (0, cb, Memory::ZERO_INIT);

			if (!InitializeSecurityDescriptor (sa_.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
				Windows::throw_last_error ();

			// Add the ACL to the security descriptor.
			if (!SetSecurityDescriptorDacl (sa_.lpSecurityDescriptor, true, acl, false))
				Windows::throw_last_error ();

			LocalFree (acl);
			acl = nullptr;

			// Initialize a security attributes structure.
			sa_.nLength = sizeof (SECURITY_ATTRIBUTES);
			psa_ = &sa_;

		} catch (...) {
			if (acl)
				LocalFree (acl);
			if (sa_.lpSecurityDescriptor)
				memory->release (sa_.lpSecurityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);
			throw;
		}
	}
}

FileSecurityAttributes::~FileSecurityAttributes ()
{
	if (sa_.lpSecurityDescriptor)
		memory->release (sa_.lpSecurityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);
}

}
}
}
