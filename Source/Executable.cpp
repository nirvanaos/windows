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
#include "../Port/Executable.h"
#include "win32.h"
#include <stdexcept>

using namespace std;

namespace Nirvana {
namespace Legacy {
namespace Core {
namespace Port {

Executable::Executable (const char* file) :
	Module ()
{
	{
		char buf [MAX_PATH + 1];
		DWORD cc = GetFullPathNameA (file, sizeof (buf), buf, nullptr);
		if (!cc || cc > sizeof (buf) - 1)
			throw runtime_error ("File not found");
		string full_path (buf, cc);
		if (!GetTempPath (sizeof (buf), buf))
			throw_UNKNOWN ();
		{
			string temp_dir = buf;
			for (;;) {
				if (!GetTempFileNameA (temp_dir.c_str (), "exe", 0, buf))
					throw_UNKNOWN ();
				if (!CopyFileA (full_path.c_str (), buf, TRUE)) {
					if (GetLastError () != ERROR_ALREADY_EXISTS)
						throw_UNKNOWN ();
				} else
					break;
			}
		}
		temp_path_ = buf;
	}

	try {
		Module::load (temp_path_.c_str ());
	} catch (...) {
		DeleteFileA (temp_path_.c_str ());
		throw;
	}
}

Executable::~Executable ()
{
	Module::unload ();
	verify (DeleteFileA (temp_path_.c_str ()));
}

}
}
}
}
