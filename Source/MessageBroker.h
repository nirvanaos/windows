#ifndef NIRVANA_CORE_WINDOWS_MESSAGEBROKER_H_
#define NIRVANA_CORE_WINDOWS_MESSAGEBROKER_H_

#include "PostOffice.h"
#include "Message.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class MessageBroker :
	public PostOffice <MessageBroker, sizeof (Message::Buffer), POSTMAN_THREAD_PRIORITY>
{
	typedef PostOffice <MessageBroker, sizeof (Message::Buffer), POSTMAN_THREAD_PRIORITY> Base;
public:
	HANDLE mailslot_handle () const
	{
		return handle_;
	}

	virtual void received (void* message, DWORD size)
	{}
};

}
}
}

#endif
