#ifndef NIRVANA_CORE_WINDOWS_MAILSLOTNAME_H_
#define NIRVANA_CORE_WINDOWS_MAILSLOTNAME_H_

#include "win32.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class MailslotName
{
public:
	MailslotName (DWORD id);

	operator LPCWSTR () const
	{
		return name_;
	}

private:
	WCHAR name_ [sizeof (MAILSLOT_PREFIX) + 8];

	static const WCHAR prefix_ [];
};

}
}
}

#endif
