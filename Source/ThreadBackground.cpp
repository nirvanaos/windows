#include <Legacy/ThreadBackground.h>
#include <ExecDomain.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

DWORD CALLBACK ThreadBackground::thread_proc (ThreadBackground* _this)
{
	Legacy::Core::ThreadBackground& thread = static_cast <Legacy::Core::ThreadBackground&> (*_this);
	try {
		thread.neutral_context().port ().convert_to_fiber ();
	} catch (...) {
		thread.execution_domain ()->on_crash ();
		thread.on_thread_proc_end ();
		return 0;
	}
	thread.port ().thread_proc ();
	thread.execution_domain ()->execute_loop ();
	thread.neutral_context ().port ().convert_to_thread ();
	thread.on_thread_proc_end ();
	return 0;
}

ThreadBackground::ThreadBackground ()
{
	if (!(event_ = CreateEventW (nullptr, 0, 0, nullptr)))
		throw_NO_MEMORY ();
}

ThreadBackground::~ThreadBackground ()
{
	CloseHandle (event_);
}

void ThreadBackground::suspend ()
{
	WaitForSingleObject (event_, INFINITE);
}

void ThreadBackground::resume ()
{
	verify (SetEvent (event_));
}

}
}
}
