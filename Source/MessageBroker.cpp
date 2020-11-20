#include "MessageBroker.h"
#include <Scheduler.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void MessageBroker::received (void* message, DWORD size) NIRVANA_NOEXCEPT
{
	const Message::Header* hdr = (const Message::Header*)message;
	switch (hdr->type) {
		case Message::Type::SHUTDOWN:
			Scheduler::shutdown ();
			break;
	}
}

}
}
}
