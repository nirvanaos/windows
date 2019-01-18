// Nirvana project.
// Windows implementation.
// Mailslot class.

#ifndef NIRVANA_CORE_WINDOWS_MAILSLOT_H_
#define NIRVANA_CORE_WINDOWS_MAILSLOT_H_

#include <Nirvana/Nirvana.h>
#include <ORB.h>
#include <Nirvana/real_copy.h>
#include <stdlib.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class Mailslot
{
public:
	Mailslot () :
		mailslot_ (INVALID_HANDLE_VALUE)
	{}

	~Mailslot ()
	{
		close ();
	}

	bool open (LPCWSTR name);

	template <size_t PREFIX_SIZE>
	bool open (const WCHAR (&prefix) [PREFIX_SIZE], DWORD id)
	{
		WCHAR name [PREFIX_SIZE + 8];
		_ultow (id, real_copy (prefix, prefix + PREFIX_SIZE - 1, name), 16);
		return open (name);
	}

	template <typename Msg>
	void send (const Msg& msg)
	{
		send (&msg, sizeof (msg));
	}

	void send (const void* msg, DWORD size)
	{
		DWORD cb;
		if (!WriteFile (mailslot_, msg, sizeof (msg), &cb, nullptr))
			throw ::CORBA::INTERNAL ();
	}

	void close ()
	{
		if (INVALID_HANDLE_VALUE != mailslot_) {
			CloseHandle (mailslot_);
			mailslot_ = INVALID_HANDLE_VALUE;
		}
	}

private:
	HANDLE mailslot_;
};

}
}
}

#endif
