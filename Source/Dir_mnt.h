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
#ifndef NIRVANA_CORE_WINDOWS_DIR_MNT_H_
#define NIRVANA_CORE_WINDOWS_DIR_MNT_H_
#pragma once

#include <NameService/Dir.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class Dir_mnt : public Nirvana::Core::Dir
{
	typedef Nirvana::Core::Dir Base;

public:
	Dir_mnt ()
	{}

	virtual StringW get_path (CosNaming::Name& n, bool create_file) const override;
	virtual void unlink (CosNaming::Name& n) const override;
	virtual void create_link (CosNaming::Name& n, const DirItemId& target, unsigned flags) const override;
	virtual DirItemId create_dir (CosNaming::Name& n, unsigned mode) const override;
	virtual std::unique_ptr <CosNaming::Core::Iterator> make_iterator () const override;
};

}
}
}
#endif
