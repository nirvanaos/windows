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

namespace ESIOP {

namespace Windows {

class OtherDomain
{
public:
	virtual SharedMemPtr reserve (size_t size);
	virtual SharedMemPtr copy (SharedMemPtr reserved, void* src, size_t& size, bool release_src);
	virtual void release (SharedMemPtr p, size_t size);
	virtual void get_sizes (PlatformSizes& sizes) NIRVANA_NOEXCEPT;
	virtual void* store_pointer (void* where, SharedMemPtr p) NIRVANA_NOEXCEPT;
	virtual void* store_size (void* where, size_t size) NIRVANA_NOEXCEPT;

	virtual void send_message (const void* msg, size_t size);

	virtual ~OtherDomain ()
	{}
};

}

/// Other protection domain communication endpoint.
class OtherDomain
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

	void send_message (const void* msg, size_t size)
	{
		implementation_->send_message (msg, size);
	}

protected:
	OtherDomain (ProtDomainId domain_id) :
		implementation_ (nullptr)
	{}

	~OtherDomain ()
	{
		delete implementation_;
	}

	Windows::OtherDomain* implementation_;
};

}

#endif
