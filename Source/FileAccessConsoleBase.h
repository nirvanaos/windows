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
#ifndef NIRVANA_CORE_PORT_FILEACCESSCONSOLEBASE_H_
#define NIRVANA_CORE_PORT_FILEACCESSCONSOLEBASE_H_
#pragma once

#include <FileAccessChar.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class FileAccessConsoleBase : public FileAccessChar
{
protected:
	FileAccessConsoleBase (FileChar* file);
	~FileAccessConsoleBase ();

	virtual void read_start () noexcept override;
	virtual void read_cancel () noexcept override;
	virtual Ref <IO_Request> write_start (const IDL::String& data) override;

private:
	void read_proc () noexcept;
	static unsigned long __stdcall s_read_proc (void*) noexcept;

	class RequestWrite : public IO_Request
	{
	public:
		RequestWrite ()
		{}

		virtual void cancel () noexcept override
		{}
	};

protected:
	void* handle_out_;
	void* handle_in_;

private:
	void* read_thread_;
	void* read_event_;
	bool read_stop_;
};

}
}
}

#endif
