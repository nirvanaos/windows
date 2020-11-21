#include "shutdown.h"
#include "Message.h"
#include "Mailslot.h"
#include "MailslotName.h"

namespace Nirvana {
namespace Core {
namespace Windows {

bool shutdown ()
{
	Mailslot ms;
	if (ms.open (MailslotName (0))) {
		try {
			ms.send (Message::Shutdown ());
			return true;
		} catch (...) {
		}
	}
	return false;
}

}
}
}
