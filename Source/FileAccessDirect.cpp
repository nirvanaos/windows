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
#include "../Port/FileAccessDirect.h"
#include "error2errno.h"
#include "MessageBroker.h"
#include <fnctl.h>

namespace Nirvana {
namespace Core {

using namespace Windows;

namespace Port {

FileAccessDirect::FileAccessDirect (File& file, unsigned flags, unsigned mode, Pos& size, Size& block_size)
{
	uint32_t creation;
	if (flags & O_CREAT) {
		if (flags & O_EXCL) {
			if (flags & O_TRUNC)
				creation = TRUNCATE_EXISTING;
			else
				creation = CREATE_NEW;
		} else if (flags & O_TRUNC)
			creation = CREATE_ALWAYS;
		else
			creation = OPEN_ALWAYS;
	} else
		creation = OPEN_EXISTING;

	// We allow Windows users to read and delete file opened by the Nirvana,
	// but do not allow write to it.
	const uint32_t SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_DELETE;

	if (!open (file, GENERIC_READ | GENERIC_WRITE, SHARE_MODE, creation, FILE_FLAG_OVERLAPPED
		| FILE_ATTRIBUTE_NORMAL
		| FILE_FLAG_NO_BUFFERING
		| FILE_FLAG_WRITE_THROUGH)
		) {

		if (flags & O_ACCMODE)
			throw_last_error ();

		if (!open (file, GENERIC_READ, SHARE_MODE, creation, FILE_FLAG_OVERLAPPED
			| FILE_ATTRIBUTE_NORMAL
			| FILE_FLAG_NO_BUFFERING
			| FILE_FLAG_WRITE_THROUGH)
			)
				throw_last_error ();
	}

	switch (creation) {
	case OPEN_ALWAYS:
	case OPEN_EXISTING: {
		LARGE_INTEGER li;
		if (!GetFileSizeEx (handle_, &li))
			throw_last_error ();
		size = li.QuadPart;
	} break;

	default:
		size = 0;
	}

	block_size = file.block_size ();
}

void FileAccessDirect::issue_request (Request& rq) noexcept
{
	switch (rq.operation ()) {
		case IO_Request::OP_SET_SIZE:
			MessageBroker::completion_port ().post (*this, rq, 0);
			return;
		case IO_Request::OP_WRITE:
			// We have to ensure that copying from this block in read() operation
			// will not cause the remapping. Otherwise it can unpredictable behavoiur
			// in kernel mode.
			// Use SRC_DECOMMIT flag to prevent changing the page states to write-copy.
			try {
				Memory::prepare_to_share (rq.buffer (), rq.size (), Nirvana::Memory::SRC_DECOMMIT);
			} catch (const CORBA::NO_MEMORY&) {
				rq.signal ({ 0, ENOMEM });
				return;
			} catch (...) {
				rq.signal ({ 0, EINVAL });
				return;
			}
			break;
	}
	Base::issue_request (rq);
}

void FileAccessDirect::completed (_OVERLAPPED* ovl, uint32_t size, uint32_t error) noexcept
{
	Request& rq = Request::from_overlapped (*ovl);
	switch (rq.operation ()) {
		case Request::OP_SET_SIZE: {
			FILE_END_OF_FILE_INFO info;
			info.EndOfFile.QuadPart = rq.offset ();
			IO_Result result = { 0 };
			if (!SetFileInformationByHandle (handle_, FileEndOfFileInfo, &info, sizeof (info)))
				result.error = error2errno (GetLastError ());
			rq.signal (result);
		} break;
		
		default:
			Base::completed (ovl, size, error);
	}
}

}
}
}