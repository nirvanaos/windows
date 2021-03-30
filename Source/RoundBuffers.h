/// \file
/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
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
#ifndef NIRVANA_CORE_WINDOWS_ROUNDBUFFERS_H_
#define NIRVANA_CORE_WINDOWS_ROUNDBUFFERS_H_

#include <Heap.h>
#include <AtomicCounter.h>

namespace Nirvana {
namespace Core {
namespace Windows {

template <class Buf>
class RoundBuffers
{
public:
	RoundBuffers (unsigned buf_cnt_min) :
		buffer_idx_ (-1)
	{
		unsigned buf_cnt = clp2 (buf_cnt_min);
		buffer_ = (Buf*)g_core_heap->allocate (nullptr, sizeof (Buf) * buf_cnt, 0);
		mask_ = (AtomicCounter <false>::IntegralType)buf_cnt - 1;
	}

	~RoundBuffers ()
	{
		g_core_heap->release (buffer_, sizeof (Buf) * (mask_ + 1));
	}

	Buf* next_buffer ()
	{
		return buffer_ + (buffer_idx_.increment () & mask_);
	}

	bool from_here (Buf* p) const
	{
		return buffer_ <= p && p < buffer_ + mask_ + 1;
	}

private:
	Buf* buffer_;
	AtomicCounter <false>::IntegralType mask_;
	AtomicCounter <false> buffer_idx_;
};

}
}
}

#endif
