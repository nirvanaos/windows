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
		buffer_ = (Buf*)g_core_heap.allocate (nullptr, sizeof (Buf) * buf_cnt, 0);
		mask_ = (AtomicCounter::UIntType)buf_cnt - 1;
	}

	~RoundBuffers ()
	{
		g_core_heap.release (buffer_, sizeof (Buf) * (mask_ + 1));
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
	AtomicCounter::UIntType mask_;
	AtomicCounter buffer_idx_;
};

}
}
}

#endif
