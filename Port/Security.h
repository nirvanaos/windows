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
#ifndef NIRVANA_CORE_PORT_SECURITY_H_
#define NIRVANA_CORE_PORT_SECURITY_H_
#pragma once

#include <Nirvana/Nirvana.h>

extern "C" __declspec (dllimport)
int __stdcall CloseHandle (void* handle);

namespace Nirvana {

typedef IDL::Sequence <CORBA::Octet> SecurityId;

namespace Core {
namespace Port {

class Thread;

/// \brief System security API.
class Security
{
public:
	typedef uint32_t ContextABI;

	class Context
	{
	public:
		Context () :
			data_ (0)
		{}

		explicit Context (ContextABI data) :
			data_ (data)
		{}

		Context (const Context& src) :
			data_ (src.duplicate ())
		{}

		Context (Context&& src) noexcept :
			data_ (src.data_)
		{
			src.data_ = 0;
		}

		~Context ()
		{
			clear ();
		}

		Context& operator = (const Context& src)
		{
			clear ();
			data_ = src.duplicate ();
			return *this;
		}

		Context& operator = (Context&& src) noexcept
		{
			clear ();
			data_ = src.data_;
			src.data_ = 0;
			return *this;
		}

		void clear () noexcept
		{
			if (data_) {
				NIRVANA_VERIFY (CloseHandle ((void*)(uintptr_t)data_));
				data_ = 0;
			}
		}

		bool empty () const noexcept
		{
			return data_ == 0;
		}

		SecurityId security_id () const;

		ContextABI abi () const noexcept
		{
			return data_;
		}

		void detach () noexcept
		{
			data_ = 0;
		}

		operator void* () const noexcept
		{
			return (void*)(uintptr_t)data_;
		}

	private:
		ContextABI duplicate () const;

	private:
		ContextABI data_;
	};

	static bool is_valid_context (ContextABI context) noexcept;

	static Context get_domain_context ();
};

}
}
}

#endif
