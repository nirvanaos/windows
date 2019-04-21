#include "ThreadMemory.h"
#include <Nirvana/real_copy.h>
#include <core.h>
#include <BackOff.h>
#include "RunnableImpl.h"
#include "../Port/ProtDomainMemory.h"
#include <assert.h>

namespace Nirvana {
namespace Core {
namespace Windows {

using namespace CORBA;

using Port::ProtDomainMemory;

inline StackInfo::StackInfo ()
{
	// Obtain stack size.
	const NT_TIB* ptib = current_TIB ();
	stack_base = (BYTE*)ptib->StackBase;
	stack_limit = (BYTE*)ptib->StackLimit;
	assert (stack_limit < stack_base);
	assert (!((SIZE_T)stack_base % PAGE_SIZE));
	assert (!((SIZE_T)stack_limit % PAGE_SIZE));

	MEMORY_BASIC_INFORMATION mbi;
#ifdef _DEBUG
	assert (VirtualQuery (stack_limit, &mbi, sizeof (mbi)));
	assert (MEM_COMMIT == mbi.State);
	assert (PAGE_READWRITE == mbi.Protect);
#endif

	// The pages before stack_limit are guard pages.
	BYTE* guard = stack_limit;
	for (;;) {
		BYTE* g = guard - PAGE_SIZE;
		verify (VirtualQuery (g, &mbi, sizeof (mbi)));
		if ((PAGE_GUARD | PAGE_READWRITE) != mbi.Protect)
			break;
		guard = g;
	}
	assert (guard < stack_limit);
	guard_begin = guard;
	allocation_base = (BYTE*)mbi.AllocationBase;
}

class ThreadMemory::StackMemory :
	protected StackInfo
{
public:
	StackMemory (const StackInfo& thread) :
		StackInfo (thread)
	{
		SIZE_T cur_stack_size = stack_base - stack_limit;

		m_tmpbuf = (BYTE*)ProtDomainMemory::space_.reserve (cur_stack_size);
		if (!m_tmpbuf)
			throw NO_MEMORY ();
		if (!VirtualAlloc (m_tmpbuf, cur_stack_size, MEM_COMMIT, PAGE_READWRITE)) {
			ProtDomainMemory::release (m_tmpbuf, cur_stack_size);
			throw NO_MEMORY ();
		}

		// Temporary copy current stack.
		real_copy ((const SIZE_T*)stack_limit, (const SIZE_T*)(stack_base), (SIZE_T*)m_tmpbuf);
	}

	~StackMemory ()
	{
		// Release temporary buffer
		SIZE_T cur_stack_size = stack_base - stack_limit;
		if (m_tmpbuf) {
			verify (VirtualFree (m_tmpbuf, cur_stack_size, MEM_DECOMMIT));
			ProtDomainMemory::release (m_tmpbuf, cur_stack_size);
		}
	}

protected:
	void finalize ();

private:
	BYTE * m_tmpbuf;
};

void ThreadMemory::StackMemory::finalize ()
{
	SIZE_T cur_stack_size = stack_base - stack_limit;

	// Commit current stack
	if (!VirtualAlloc (stack_limit, cur_stack_size, MEM_COMMIT, PageState::RW_MAPPED_PRIVATE))
		throw NO_MEMORY ();

	// Commit guard page(s)
	if (!VirtualAlloc (guard_begin, stack_limit - guard_begin, MEM_COMMIT, PAGE_GUARD | PageState::RW_MAPPED_PRIVATE))
		throw NO_MEMORY ();

	// Copy current stack contents back.
	real_copy ((SIZE_T*)m_tmpbuf, (SIZE_T*)m_tmpbuf + cur_stack_size / sizeof (SIZE_T), (SIZE_T*)stack_limit);
}

class ThreadMemory::StackPrepare :
	private StackMemory
{
public:
	StackPrepare (const ThreadMemory& thread) :
		StackMemory (thread),
		m_finalized (true)
	{
		m_mapped_end = allocation_base;
	}

	~StackPrepare ()
	{
		if (!m_finalized) {	// On error.
			// Unmap and free mapped blocks.
			BYTE* ptail = m_mapped_end + ALLOCATION_GRANULARITY;
			if (ptail < stack_base)
				VirtualFree (ptail, 0, MEM_RELEASE); // May fail. No verify.
			VirtualFree (m_mapped_end, 0, MEM_RELEASE); // May fail. No verify.
			while (m_mapped_end > allocation_base) {
				ProtDomainMemory::Block (m_mapped_end -= ALLOCATION_GRANULARITY).unmap ();
				verify (VirtualFree (m_mapped_end, 0, MEM_RELEASE));
			}

			// Reserve Windows stack back.
			verify (VirtualAlloc (allocation_base, stack_base - allocation_base, MEM_RESERVE, PAGE_READWRITE));

			try {
				finalize ();
			} catch (...) {
			}
		}
	}

	void run ()
	{
		prepare ();
	}

private:
	void prepare ()
	{
		m_finalized = false;

		// Mark memory as reserved.
		for (BYTE* p = allocation_base; p < stack_base; p += ALLOCATION_GRANULARITY)
			ProtDomainMemory::space_.block (p).mapping = INVALID_HANDLE_VALUE;

		// Decommit Windows memory
		verify (VirtualFree (guard_begin, stack_base - guard_begin, MEM_DECOMMIT));

#ifdef _DEBUG
		{
			MEMORY_BASIC_INFORMATION dmbi;
			ProtDomainMemory::query (allocation_base, dmbi);
			assert (dmbi.State == MEM_RESERVE);
			assert (allocation_base == dmbi.AllocationBase);
			assert (allocation_base == dmbi.BaseAddress);
			assert (((BYTE*)dmbi.BaseAddress + dmbi.RegionSize) == stack_base);
		}
#endif

		// Map all stack, but not commit.
		for (; m_mapped_end < stack_base; m_mapped_end += ALLOCATION_GRANULARITY) {
			ProtDomainMemory::Block (m_mapped_end).commit (0, 0);
		}

		finalize ();
		m_finalized = true;
	}

private:
	BYTE * m_mapped_end;
	bool m_finalized;
};

class ThreadMemory::StackUnprepare :
	private StackMemory
{
public:
	StackUnprepare (const ThreadMemory& thread) :
		StackMemory (thread)
	{}

	void run ()
	{
		unprepare ();
	}

private:
	void unprepare ()
	{
		// Remove mappings and free memory. But blocks still marekd as reserved.
		for (BYTE* p = allocation_base; p < stack_base; p += ALLOCATION_GRANULARITY) {
			HANDLE mapping = InterlockedExchangePointer (&ProtDomainMemory::space_.allocated_block (p)->mapping, INVALID_HANDLE_VALUE);
			if (mapping != INVALID_HANDLE_VALUE) {
				verify (UnmapViewOfFile (p));
				verify (CloseHandle (mapping));
			} else if (!mapping)
				ProtDomainMemory::space_.allocated_block (p)->mapping = INVALID_HANDLE_VALUE;
			else
				VirtualFree (p, 0, MEM_RELEASE);
		}

		// Reserve all stack.
		for (Core::BackOff bo;
				 !VirtualAlloc (allocation_base, stack_base - allocation_base, MEM_RESERVE, PAGE_READWRITE);
				 bo.sleep ()) {
			assert (ERROR_INVALID_ADDRESS == GetLastError ());
		}

		// Mark blocks as free
		for (BYTE* p = allocation_base; p < stack_base; p += ALLOCATION_GRANULARITY)
			ProtDomainMemory::space_.allocated_block (p)->mapping = 0;

		finalize ();
	}
};

template <class T>
class ThreadMemory::Runnable :
	public RunnableImpl <ThreadMemory::Runnable <T> >
{
public:
	Runnable (const ThreadMemory& thread) :
		thread_ (thread)
	{}

	void run ()
	{
		T (thread_).run ();
	}

private:
	const ThreadMemory& thread_;
};

ThreadMemory::ThreadMemory () :
	StackInfo ()
{ // Prepare stack of current thread to share.
	// Call stack_prepare in fiber
	run_in_neutral_context (&Runnable <StackPrepare> (*this));
	_set_se_translator (&ProtDomainMemory::se_translator);
}

ThreadMemory::~ThreadMemory ()
{
	run_in_neutral_context (&Runnable <StackUnprepare> (*this));
}

}
}
}
