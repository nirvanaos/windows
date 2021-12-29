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
#include "../Port/FileAccess.h"
#include "SchedulerBase.h"
#include "error2errno.h"
#include <Nirvana/RuntimeError.h>

namespace Nirvana {
namespace Core {

namespace Windows {

void FileAccess::open (const std::string& path, uint32_t access, uint32_t share_mode, uint32_t creation_disposition, uint32_t flags_and_attributes)
{
	std::basic_string <WinWChar> wpath;
	utf8_to_ucs16 (path, wpath);
	handle_ = CreateFileW (wpath.c_str (), access, share_mode, nullptr, creation_disposition, flags_and_attributes, nullptr);
	if (INVALID_HANDLE_VALUE == handle_)
		throw RuntimeError (error2errno (GetLastError ()));
	SchedulerBase::singleton ().completion_port ().add_receiver (handle_, *this);
}

FileAccess::~FileAccess ()
{
	if (INVALID_HANDLE_VALUE != handle_)
		CloseHandle (handle_);
}

void FileAccess::completed (_OVERLAPPED* ovl, uint32_t size, uint32_t error) NIRVANA_NOEXCEPT
{
	IO_Result result { size, 0 };
	if (error)
		result.error = error2errno (error);
	Request::from_overlapped (*ovl).set (result);
}

void FileAccess::issue_request (Request& rq) NIRVANA_NOEXCEPT
{
	BOOL ret;
	switch (rq.operation ()) {
		case Request::OP_READ:
			ret = ReadFile (handle_, rq.buffer (), rq.size (), nullptr, rq);
			break;
		case Request::OP_WRITE:
			ret = WriteFile (handle_, rq.buffer (), rq.size (), nullptr, rq);
			break;
		default:
			ret = TRUE;
			rq.set ({ 0, ENOTSUP });
	}
	if (!ret) {
		DWORD err = GetLastError ();
		if (ERROR_IO_PENDING != err)
			rq.set ({ 0, error2errno (err) });
	}
}

}

using namespace Windows;

namespace Port {

FileAccessDirect::FileAccessDirect (const std::string& path, int flags, Pos& size, Size& block_size)
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

	open (path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, creation,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED);

	LARGE_INTEGER li;
	if (GetFileSizeEx (handle_, &li))
		size = li.QuadPart;
	throw RuntimeError (error2errno (GetLastError ()));

	block_size = 4096; // TODO: Implement
}

void FileAccessDirect::issue_request (Request& rq) NIRVANA_NOEXCEPT
{
	if (rq.operation () == IO_Request::OP_SET_SIZE)
		SchedulerBase::singleton ().completion_port ().post (*this, rq, 0);
	else
		Base::issue_request (rq);
}

void FileAccessDirect::completed (_OVERLAPPED* ovl, uint32_t size, uint32_t error) NIRVANA_NOEXCEPT
{
	Request& rq = Request::from_overlapped (*ovl);
	if (rq.operation () == Request::OP_SET_SIZE) {
		FILE_END_OF_FILE_INFO info;
		info.EndOfFile.QuadPart = rq.offset ();
		IO_Result res = { 0 };
		if (!SetFileInformationByHandle (handle_, FileEndOfFileInfo, &info, sizeof (info)))
			res.error = error2errno (GetLastError ());
		rq.set (res);
	} else
		Base::completed (ovl, size, error);
}

}
}
}
