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

	Mailslot (Mailslot&& src) :
		mailslot_ (src.mailslot_)
	{
		src.mailslot_ = nullptr;
	}

	~Mailslot ()
	{
		close ();
	}

	bool open (const wchar_t* name);

	template <typename Msg>
	void send (const Msg& msg)
	{
		send (&msg, sizeof (msg));
	}

	void send (const void* msg, uint32_t size);

	void close ();

	bool is_open () const
	{
		return mailslot_ != (void*)(intptr_t)-1;
	}

private:
	void* mailslot_;
};

}
}
}

#endif
