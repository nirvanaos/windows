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
#ifndef NIRVANA_ESIOP_PORT_OTHERDOMAIN_H_
#define NIRVANA_ESIOP_PORT_OTHERDOMAIN_H_
#pragma once

#include <ORB/ESIOP.h>
#include "../Source/Mailslot.h"
#include "../Source/OtherSpace.h"

//#define NIRVANA_SINGLE_PLATFORM

namespace ESIOP {

namespace Windows {

class Mailslot : public Nirvana::Core::Windows::Mailslot
{
	typedef Nirvana::Core::Windows::Mailslot Base;

public:
	void send_message (const void* msg, size_t size)
	{
		Base::send (msg, (uint32_t)size);
	}

protected:
	Mailslot (ProtDomainId domain_id);
};

}

#ifdef NIRVANA_SINGLE_PLATFORM

/// Other protection domain communication endpoint.
class OtherDomain :
	public ESIOP::Windows::OtherSpace <sizeof (void*) == 8>,
	public ESIOP::Windows::Mailslot
{
	typedef ESIOP::Windows::OtherSpace <sizeof (void*) == 8> Space;

public:
	static const bool slow_creation = true;

	SharedMemPtr reserve (size_t size);
	SharedMemPtr copy (SharedMemPtr reserved, void* src, size_t& size, bool release_src);
	void release (SharedMemPtr p, size_t size);

	static void get_sizes (PlatformSizes& sizes) NIRVANA_NOEXCEPT
	{
		Space::get_sizes (sizes);
	}

	static void* store_pointer (void* where, SharedMemPtr p) NIRVANA_NOEXCEPT
	{
		return Space::store_pointer (where, p);
	}

	static void* store_size (void* where, size_t size) NIRVANA_NOEXCEPT
	{
		return Space::store_size (where, size);
	}

protected:
	OtherDomain (ProtDomainId domain_id);
};

#else

namespace Windows {

class OtherDomain
{
public:
	virtual SharedMemPtr reserve (size_t size) = 0;
	virtual SharedMemPtr copy (SharedMemPtr reserved, void* src, size_t& size, bool release_src) = 0;
	virtual void release (SharedMemPtr p, size_t size) = 0;
	virtual void get_sizes (PlatformSizes& sizes) NIRVANA_NOEXCEPT = 0;
	virtual void* store_pointer (void* where, SharedMemPtr p) NIRVANA_NOEXCEPT = 0;
	virtual void* store_size (void* where, size_t size) NIRVANA_NOEXCEPT = 0;

	virtual ~OtherDomain ()
	{}
};

}

/// Other protection domain communication endpoint.
class OtherDomain :
	public Windows::Mailslot
{
public:
	static const bool slow_creation = true;

	SharedMemPtr reserve (size_t size)
	{
		return implementation_->reserve (size);
	}

	SharedMemPtr copy (SharedMemPtr reserved, void* src, size_t& size, bool release_src)
	{
		return implementation_->copy (reserved, src, size, release_src);
	}

	void release (SharedMemPtr p, size_t size)
	{
		implementation_->release (p, size);
	}

	void get_sizes (PlatformSizes& sizes) NIRVANA_NOEXCEPT
	{
		implementation_->get_sizes (sizes);
	}

	void* store_pointer (void* where, SharedMemPtr p) NIRVANA_NOEXCEPT
	{
		return implementation_->store_pointer (where, p);
	}

	void* store_size (void* where, size_t size) NIRVANA_NOEXCEPT
	{
		return implementation_->store_size (where, size);
	}

protected:
	OtherDomain (ProtDomainId domain_id);

	~OtherDomain ()
	{
		delete implementation_;
	}

	Windows::OtherDomain* implementation_;
};

#endif

}

#endif
