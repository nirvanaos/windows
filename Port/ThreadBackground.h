// Nirvana project
// Windows implementation.
// ThreadBackgroundBase class.
// Platform-specific background thread implementation.

#ifndef NIRVANA_CORE_PORT_THREADBACKGROUND_H_
#define NIRVANA_CORE_PORT_THREADBACKGROUND_H_

#include <Thread.h>

struct _SECURITY_ATTRIBUTES;

extern "C" __declspec (dllimport)
void* __stdcall CreateEventW (_SECURITY_ATTRIBUTES*, int bManualReset, int bInitialState, const wchar_t* lpName);

namespace Nirvana {
namespace Core {
namespace Port {

class ThreadBackground :
	public Core::Thread
{
public:
	ThreadBackground ()
	{
		if (!(event_ = CreateEventW (nullptr, 0, 0, nullptr)))
			throw_NO_MEMORY ();
	}

	~ThreadBackground ()
	{
		CloseHandle (event_);
	}

	virtual Core::ExecContext* neutral_context ();

protected:
	void create ();
	void suspend ();
	void resume ();

private:
	friend class Nirvana::Core::Port::Thread;
	static unsigned long __stdcall thread_proc (ThreadBackground* _this);

private:
	Core::ExecContext neutral_context_;
	void* event_;
};

}
}
}

#endif
