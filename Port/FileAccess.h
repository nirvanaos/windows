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
#ifndef NIRVANA_CORE_PORT_FILEACCESS_H_
#define NIRVANA_CORE_PORT_FILEACCESS_H_

#include <Nirvana/Nirvana.h>
#include <IO_Request.h>
#include "../Source/CompletionPortReceiver.h"

#define	O_CREAT 0x0200
#define	O_TRUNC 0x0400
#define	O_EXCL  0x0800

namespace Nirvana {
namespace Core {

namespace Windows {

class FileAccess :
	protected CompletionPortReceiver
{
protected:
	// The same as Windows OVERLAPPED structure
	struct Overlapped
	{
		uintptr_t Internal;
		uintptr_t InternalHigh;
		union
		{
			struct
			{
				uint32_t Offset;
				uint32_t OffsetHigh;
			} DUMMYSTRUCTNAME;
			void* Pointer;
		} DUMMYUNIONNAME;

		void* hEvent;

		Overlapped () NIRVANA_NOEXCEPT
		{
			memset (this, 0, sizeof (*this));
		}
	};

	class Request :
		protected Overlapped,
		public IO_Request
	{
	public:
		Request (Operation op, void* buf, uint32_t size) NIRVANA_NOEXCEPT :
			IO_Request (op)
		{
			Internal = (uintptr_t)buf;
			InternalHigh = size;
		}

		void* buffer () const NIRVANA_NOEXCEPT
		{
			return (void*)Internal;
		}

		uint32_t size () const NIRVANA_NOEXCEPT
		{
			return (uint32_t)InternalHigh;
		}

		operator _OVERLAPPED* () NIRVANA_NOEXCEPT
		{
			return &reinterpret_cast <_OVERLAPPED&> (static_cast <Overlapped&> (*this));
		}

		static Request& from_overlapped (_OVERLAPPED& ovl) NIRVANA_NOEXCEPT
		{
			return static_cast <Request&> (reinterpret_cast <Overlapped&> (ovl));
		}
	};

	FileAccess () :
		handle_ ((void*)-1)
	{}

	void open (const std::string& path, uint32_t access, uint32_t share_mode, uint32_t creation_disposition, uint32_t flags_and_attributes);
	~FileAccess ();

	void issue_request (Request& rq) NIRVANA_NOEXCEPT;

private:
	virtual void completed (_OVERLAPPED* ovl, uint32_t size, uint32_t error) NIRVANA_NOEXCEPT;

protected:
	void* handle_;
};

}

namespace Port {

/// Interface to host (kernel) filesystem driver.
class FileAccessDirect : public Nirvana::Core::Windows::FileAccess
{
	typedef Nirvana::Core::Windows::FileAccess Base;
	typedef Base::Request RequestBase;
protected:
	typedef uint64_t Pos;      ///< File position type.
	typedef uint32_t Size;     ///< R/W block size type.
	typedef uint64_t BlockIdx; ///< Block index type. Must fit maximal position / minimal block_size.

	/// I/O request.
	/// Must derive Core::IO_Request.
	class Request :
		public RequestBase
	{
		typedef RequestBase Base;
	public:
		/// Constructor.
		/// 
		/// \param op I/O operation.
		/// \param offset R/W start offset. Must be aligned on the block boundary.
		/// \param buf R/W buffer.
		/// \param size R/W byte count.
		Request (Operation op, Pos offset, void* buf, Size size) NIRVANA_NOEXCEPT :
			Base (op, buf, size)
		{
			DUMMYUNIONNAME.DUMMYSTRUCTNAME.Offset = (uint32_t)offset;
			DUMMYUNIONNAME.DUMMYSTRUCTNAME.OffsetHigh = (offset >> 32);
		}
	};

	/// Constructor.
	/// 
	/// \param path File name, UTF-8 encoded.
	/// \param flags Creation flags.
	/// \throw RuntimeError.
	FileAccessDirect (const std::string& path, int flags);

	/// Gets block size.
	/// Called once in the derived constructor.
	/// 
	/// \returns Block (cluster) size.
	/// \throw On some platforms may throw ReutimeError.
	Size block_size () NIRVANA_NOEXCEPT
	{
		return 4096; // TODO: Implement
	}

	/// \returns File size.
	/// \throw `RuntimeError`.
	Pos file_size ();

	/// Sets file size.
	/// Used for file truncation.
	/// 
	/// \param new_size The new file size in bytes.
	/// \throw RuntimeError.
	void file_size (Pos new_size);

	/// Issues the I/O request to the host or kernel.
	/// 
	/// \param rq The `Request` object.
	void issue_request (Request& rq) NIRVANA_NOEXCEPT
	{
		Base::issue_request (rq);
	}
};

}
}
}

#endif
