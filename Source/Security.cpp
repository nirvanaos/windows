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
#include "../Port/Security.h"
#include "win32.h"
#include "error2errno.h"

namespace Nirvana {
namespace Core {

using Windows::throw_last_error;

namespace Port {

Security::ContextABI Security::Context::duplicate () const
{
	if (!data_)
		return 0;

	HANDLE process = GetCurrentProcess ();
	HANDLE h;
	if (!DuplicateHandle (process, (HANDLE)(uintptr_t)data_, process, &h, 0, false, DUPLICATE_SAME_ACCESS))
		throw_last_error ();
	assert ((uintptr_t)h <= 0xFFFFFFFF);
	return (ContextABI)(uintptr_t)h;
}

SecurityId Security::Context::security_id () const
{
	if (!data_)
		throw_BAD_PARAM ();

	DWORD len = 0;
	GetTokenInformation (*this, TokenUser, nullptr, 0, &len);
	if (!len)
		throw_last_error ();
	std::vector <uint8_t> buf ((size_t)len);
	if (!GetTokenInformation (*this, TokenUser, buf.data (), len, &len))
		throw_last_error ();

	PSID sid = ((const TOKEN_USER*)buf.data ())->User.Sid;
	len = GetLengthSid (sid);
	return SecurityId ((const CORBA::Octet*)sid, (const CORBA::Octet*)sid + len);
}

bool Security::is_valid_context (ContextABI context) noexcept
{
	if (!context)
		return false;

	DWORD len = 0;
	GetTokenInformation ((HANDLE)(uintptr_t)context, TokenUser, nullptr, 0, &len);
	return len != 0;
}

Security::Context Security::get_domain_context ()
{
	HANDLE token;
	if (!OpenProcessToken (GetCurrentProcess (), TOKEN_READ | TOKEN_DUPLICATE | TOKEN_IMPERSONATE, &token))
		Windows::throw_last_error ();
	return Context ((ContextABI)(uintptr_t)token);
}

}
}
}
