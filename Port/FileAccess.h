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
#include <IO_WaitList.h>
#include "../Source/CompletionPortReceiver.h"

#define	O_CREAT 0x0200
#define	O_TRUNC 0x0400
#define	O_EXCL  0x0800

namespace Nirvana {
namespace Core {

namespace Windows {

class NIRVANA_NOVTABLE FileAccess :
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

    Overlapped ()
    {
      memset (this, 0, sizeof (*this));
    }
  };

  class IORBase :
    public Overlapped
  {};

  FileAccess () :
    handle_ ((void*)-1)
  {}

  void open (const std::string& path, uint32_t access, uint32_t share_mode, uint32_t creation_disposition, uint32_t flags_and_attributes);
  ~FileAccess ();

protected:
  void* handle_;
};

}

namespace Port {

/// Direct file access interface to host (kernel).
class FileAccessDirect : public Nirvana::Core::Windows::FileAccess
{
public:
  class IO_Request :
    public IO_WaitList,
    public Nirvana::Core::Windows::FileAccess::IORBase
  {};

  /// Constructor
	/// \param path File name, UTF-8 encoded.
	/// \param flags Creation flags.
  FileAccessDirect (const std::string& path, int flags);

  size_t block_size ()
  {
    return 0x10000;
  }

  uint64_t file_size ();

	void start_read (uint64_t pos, uint32_t size, void* buf, IO_Request& ior) NIRVANA_NOEXCEPT;
  void start_write (uint64_t pos, uint32_t size, const void* buf, IO_Request& ior) NIRVANA_NOEXCEPT;

private:
  virtual void completed (_OVERLAPPED* ovl, uint32_t size, uint32_t error) NIRVANA_NOEXCEPT;
};

class FileAccessSequential : public Nirvana::Core::Windows::FileAccess
{
public:
};

}
}
}

#endif
