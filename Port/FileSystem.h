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
#ifndef NIRVANA_CORE_PORT_FILESYSTEM_H_
#define NIRVANA_CORE_PORT_FILESYSTEM_H_
#pragma once

#include <Nirvana/FS.h>
#include <CORBA/Server.h>
#include <CORBA/CosNaming.h>

namespace Nirvana {
namespace Core {
namespace Port {

class Dir
{
protected:
	Dir (const PortableServer::ObjectId& id);

	class Iterator
	{
	public:
		Iterator (Dir& dir);
		~Iterator ();

		Iterator& (Iterator&& src) :
			handle_ (src.handle)
		{
			handle_ = src.handle_;
			src.handle_ = nullptr;
			return *this;
		}

		bool next (Cos::Name::Binding& binding);

	private:
		void* handle_;
	};

private:
	void* handle_;
};

class FileSystem
{
public:
	static ::PortableServer::ObjectId get_unique_id (const CosNaming::Name& name);

	static CosNaming::BindingType get_binding_type (PortableServer::ObjectId& id);
};

}
}
}

#endif
