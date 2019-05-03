// Nirvana project.
// Windows implementation.
// Mailslot class.

#ifndef NIRVANA_CORE_WINDOWS_MAILSLOT_H_
#define NIRVANA_CORE_WINDOWS_MAILSLOT_H_

#include <Nirvana/Nirvana.h>
#include <Nirvana/real_copy.h>
#include <stdlib.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class Mailslot
{
public:
	Mailslot () :
		mailslot_ ((void*)(intptr_t)-1)
	{}

	~Mailslot ()
	{
		close ();
	}

	bool open (const wchar_t* name);

	template <size_t PREFIX_SIZE>
	bool open (const wchar_t (&prefix) [PREFIX_SIZE], uint32_t id)
	{
		wchar_t name [PREFIX_SIZE + 8];
		_ultow (id, real_copy (prefix, prefix + PREFIX_SIZE - 1, name), 16);
		return open (name);
	}

	template <typename Msg>
	void send (const Msg& msg)
	{
		send (&msg, sizeof (msg));
	}

	void send (const void* msg, uint32_t size);

	void close ();

private:
	void* mailslot_;
};

}
}
}

#endif
