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
#include "pch.h"
#include "../Port/Security.h"
#include "error2errno.h"
#include "TokenUser.h"
#include <Nirvana/string_conv.h>

namespace Nirvana {
namespace Core {

namespace Port {

void* Security::process_token_;
unsigned Security::everyone_ [SECURITY_MAX_SID_SIZE / sizeof (unsigned)];

bool Security::initialize () noexcept
{
	if (!OpenProcessToken (GetCurrentProcess (), TOKEN_READ | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
		&process_token_)
		)
		return false;
	
	DWORD cb = sizeof (everyone_);
	if (!CreateWellKnownSid (WinWorldSid, nullptr, &everyone_, &cb))
		return false;

	return true;
}

void Security::terminate () noexcept
{
	CloseHandle (process_token_);
}

Security::Context::ABI Security::Context::duplicate () const
{
	if (!data_)
		return 0;

	HANDLE process = GetCurrentProcess ();
	HANDLE h;
	if (!DuplicateHandle (process, (HANDLE)(uintptr_t)data_, process, &h, 0, false, DUPLICATE_SAME_ACCESS))
		Windows::throw_last_error ();
	assert ((uintptr_t)h <= 0xFFFFFFFF);
	return (ABI)(uintptr_t)h;
}

SecurityId Security::Context::security_id () const
{
	if (!data_)
		throw_BAD_PARAM ();

	Windows::TokenUser user (*this);

	return make_security_id (user->User.Sid);
}

bool Security::is_valid_context (Context::ABI context) noexcept
{
	if (!context)
		return false;

	DWORD len = 0;
	GetTokenInformation ((HANDLE)(uintptr_t)context, TokenUser, nullptr, 0, &len);
	return len != 0;
}

SecurityId Security::make_security_id (PSID sid)
{
	size_t len = GetLengthSid (sid);
	assert (len);
	return SecurityId ((const CORBA::Octet*)sid, (const CORBA::Octet*)sid + len);
}

class NameBuf
{
public:
	NameBuf () :
		ptr_ (static_),
		size_ (std::size (static_))
	{}

	size_t size () const noexcept
	{
		return size_;
	}

	void size (size_t cc)
	{
		dynamic_.resize (cc - 1);
		ptr_ = const_cast <Windows::WinWChar*> (dynamic_.data ());
		size_ = cc;
	}

	Windows::WinWChar* ptr () const noexcept
	{
		return ptr_;
	}

private:
	Windows::WinWChar static_ [128];
	Windows::StringW dynamic_;
	Windows::WinWChar* ptr_;
	size_t size_;
};

IDL::String Security::get_name (const SecurityId& id)
{
	NameBuf name, domain;
	SID_NAME_USE use;
	DWORD name_cc, domain_cc;
	for (;;) {
		name_cc = (DWORD)name.size ();
		domain_cc = (DWORD)domain.size ();
		if (!LookupAccountSidW (nullptr, (PSID)id.data (), name.ptr (), &name_cc, domain.ptr (), &domain_cc, &use)) {
			DWORD err = GetLastError ();
			if (ERROR_INSUFFICIENT_BUFFER != err)
				Windows::throw_win_error_sys (err);
		} else
			break;

		if (name_cc > name.size ())
			name.size (name_cc);
		if (domain_cc > domain.size ())
			domain.size (domain_cc);
	}

	IDL::String ret;
	ret.reserve (name_cc + 1 + domain_cc);
	append_utf8 (name.ptr (), name.ptr () + name_cc, ret);
	ret += '/';
	append_utf8 (domain.ptr (), domain.ptr () + domain_cc, ret);
	return ret;
}

}
}
}
