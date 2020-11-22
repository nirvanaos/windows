#include <Legacy/ThreadBackground.h>
#include <ExecDomain.h>
#include "Thread.inl"

namespace Nirvana {
namespace Core {
namespace Port {

DWORD CALLBACK ThreadBackground::thread_proc (ThreadBackground* _this)
{
	Legacy::Core::ThreadBackground& thread = static_cast <Legacy::Core::ThreadBackground&> (*_this);
	Port::Thread::current (thread);
	try {
		thread.neutral_context().port ().convert_to_fiber ();
	} catch (...) {
		thread.exec_domain ()->on_exec_domain_crash (CORBA::SystemException::EC_NO_MEMORY);
		thread.on_thread_proc_end ();
		return 0;
	}
	thread.exec_domain ()->switch_to ();
	Core::ExecContext::neutral_context_loop ();
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

void ThreadBackground::create ()
{
	Thread::create (this, Windows::BACKGROUND_THREAD_PRIORITY);
}

void ThreadBackground::suspend ()
{
	WaitForSingleObject (event_, INFINITE);
}

void ThreadBackground::resume ()
{
	verify (SetEvent (event_));
}

/// Temparary boost thread priority above the worker threads priority.
void ThreadBackground::priority_boost ()
{
	SetThreadPriority (handle_, Windows::BACKGROUND_THREAD_PRIORITY_BOOSTED);
}

void ThreadBackground::priority_restore ()
{
	SetThreadPriority (handle_, Windows::BACKGROUND_THREAD_PRIORITY);
}

}
}
}
