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
#ifndef NIRVANA_CORE_PORT_DIR_H_
#define NIRVANA_CORE_PORT_DIR_H_
#pragma once

#include <NameService/NamingContextRoot.h>
#include "../Source/DirItem.h"

namespace Nirvana {
namespace Core {
namespace Port {

/// Implements Dir object for host system.
class NIRVANA_NOVTABLE Dir : 
	public Windows::DirItem,
	public CosNaming::Core::NamingContextRoot
{
	typedef Windows::DirItem Base;

	///@{
	/// DirItem implementation.

public:
	/// File system object type.
	FileType type () const noexcept
	{
		return Base::type ();
	}

	/// Existance check
	bool _non_existent () const noexcept
	{
		if (type () == FileType::not_found)
			return true;
		return Base::_non_existent ();
	}

	/// File system object status information.
	/// 
	/// \param [out] st FileStat structure.
	void stat (FileStat& st)
	{
		Base::stat (st);
	}

protected:
	/// File system object unique id.
	const DirItemId& id () const noexcept
	{
		return Base::id ();
	}

	/// Etherealize file system object: close all handles etc.
	void etherealize () noexcept
	{
		Base::etherealize ();
	}

	/// Delete all links to a file or empty directory.
	virtual void remove ();

	///@}

protected:

	///@{
	/// Dir implementation.
	
	/// Bind file.
	/// 
	/// \param n Name.
	/// \param file File object unique id.
	/// \param rebind `true` for rebind.
	void bind_file (CosNaming::Name& n, const DirItemId& file, bool rebind) const
	{
		create_link (n, file, rebind ? FLAG_REBIND : 0);
	}

	/// Bind directory.
	/// 
	/// \param n Name.
	/// \param file Dir object unique id.
	/// \param rebind `true` for rebind.
	void bind_dir (CosNaming::Name& n, const DirItemId& dir, bool rebind) const
	{
		create_link (n, dir, FLAG_DIRECTORY | (rebind ? FLAG_REBIND : 0));
	}

	/// Resolve path to object.
	/// This method must be really const to avoid race condition in iterator.
	/// \param n File system object name.
	/// \returns File system object unique id.
	DirItemId resolve_path (CosNaming::Name& n) const;

	/// Unlink object.
	/// 
	/// \param n Object name.
	virtual void unlink (CosNaming::Name& n) const;

	/// Create directory.
	/// 
	/// \param n Directory name.
	/// \returns New directory unique id.
	virtual DirItemId create_dir (CosNaming::Name& n, unsigned mode) const;

	/// Make new file id.
	/// 
	/// \param n File name.
	/// \returns File id.
	DirItemId get_new_file_id (CosNaming::Name& n) const;

	/// Make directory iterator.
	/// 
	/// \returns Unique pointer to CosNaming::Core::Iterator object.
	virtual std::unique_ptr <CosNaming::Core::Iterator> make_iterator () const override;

	unsigned check_access (CosNaming::Name& n) const;

	///@}

protected:
	// Implementation details.
	Dir (const DirItemId& id);
	Dir ();

	// Append first name component if it not last or dont_append_last=false.
	// May be overriden.
	virtual Windows::StringW get_path (CosNaming::Name& n, bool dont_append_last) const;

	static void append_path (Windows::StringW& path, const CosNaming::NameComponent& nc);
	virtual void create_link (CosNaming::Name& n, const DirItemId& target, unsigned flags) const;

	Windows::StringW get_pattern () const;

private:
	Windows::StringW make_path () const;
	Windows::StringW check_path (CosNaming::Name& n, bool dont_append_last = false) const;

	static const unsigned FLAG_REBIND = 0x80000000;
	static const unsigned FLAG_DIRECTORY = 0x00000001;

	static void unlink (const Windows::WinWChar* path, uint32_t att);
};

}
}
}

#endif
