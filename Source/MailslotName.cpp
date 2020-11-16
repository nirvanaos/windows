#include <Nirvana/real_copy.h>
#include "MailslotName.h"
#include <stdlib.h>

namespace Nirvana {
namespace Core {
namespace Windows {

const WCHAR MailslotName::prefix_ [sizeof (MAILSLOT_PREFIX)] = MAILSLOT_PREFIX;

MailslotName::MailslotName (DWORD id)
{
	_ultow (id, real_copy (prefix_, prefix_ + _countof (prefix_) - 1, name_), 16);
}

}
}
}
