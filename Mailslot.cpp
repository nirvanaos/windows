// Nirvana project.
// Windows implementation.
// Mailslot class.

#include "Mailslot.h"

namespace Nirvana {
namespace Core {
namespace Windows {

bool Mailslot::open (LPCWSTR name)
{
	assert (INVALID_HANDLE_VALUE == mailslot_);
	mailslot_ = CreateFileW (name, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (INVALID_HANDLE_VALUE == mailslot_) {
		DWORD err = GetLastError ();
		if (ERROR_FILE_NOT_FOUND)
			return false;
		throw ::CORBA::INTERNAL ();
	}
	return true;
}

}
}
}
