#include <Legacy/ThreadBackground.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Port {

DWORD CALLBACK ThreadBackground::thread_proc (ThreadBackground* _this)
{
	try {
		_this->neutral_context_.port ().convert_to_fiber ();
	} catch (...) {
		_this->execution_domain ()->on_crash ();
		static_cast <Nirvana::Legacy::Core::ThreadBackground*> (_this)->on_thread_proc_end ();
		return 0;
	}
	_this->context (&_this->neutral_context_);
	_this->port ().thread_proc ();
	_this->execution_domain ()->execute_loop ();
	_this->neutral_context_.port ().convert_to_thread ();
	static_cast <Nirvana::Legacy::Core::ThreadBackground*> (_this)->on_thread_proc_end ();
	return 0;
}

Core::ExecContext* ThreadBackground::neutral_context ()
{
	return &neutral_context_;
}

void ThreadBackground::create ()
{
	port ().create (this, THREAD_PRIORITY_NORMAL);
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
