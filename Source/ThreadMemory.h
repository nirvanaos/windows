#ifndef NIRVANA_CORE_WINDOWS_THREADMEMORY_H_
#define NIRVANA_CORE_WINDOWS_THREADMEMORY_H_

#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

struct StackInfo
{
	BYTE* stack_base;
	BYTE* stack_limit;
	BYTE* guard_begin;
	BYTE* allocation_base;

	StackInfo ();
};

class ThreadMemory :
	protected StackInfo
{
public:
	ThreadMemory ();
	~ThreadMemory ();

private:
	class StackMemory;
	class StackPrepare;
	class StackUnprepare;
	template <class T> class Runnable;
};

}
}
}

#endif