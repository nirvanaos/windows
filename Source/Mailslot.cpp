// Nirvana project.
// Windows implementation.
// Mailslot class.

#include "Mailslot.h"
#include <CORBA/Exception.h>
#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

bool Mailslot::open (const wchar_t* name)
{
	assert (INVALID_HANDLE_VALUE == mailslot_);
	mailslot_ = CreateFileW (name, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE == mailslot_) {
		DWORD err = GetLastError ();
		if (ERROR_FILE_NOT_FOUND == err)
			return false;
		throw_INTERNAL ();
	}
	return true;
}

void Mailslot::close ()
{
	if (INVALID_HANDLE_VALUE != mailslot_) {
		CloseHandle (mailslot_);
		mailslot_ = INVALID_HANDLE_VALUE;
	}
}

void Mailslot::send (const void* msg, uint32_t size)
{
	DWORD cb;
	if (!WriteFile (mailslot_, msg, sizeof (msg), &cb, nullptr))
		throw_COMM_FAILURE ();
}

}
}
}
