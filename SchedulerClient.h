#ifndef NIRVANA_CORE_WINDOWS_SCHEDULERCLIENT_H_
#define NIRVANA_CORE_WINDOWS_SCHEDULERCLIENT_H_

#include "SchedulerIPC.h"
#include "Mailslot.h"
#include "MailslotReader.h"

namespace Nirvana {
namespace Core {
namespace Windows {

class SchedulerClient :
	private SchedulerIPC,
	public MailslotReader
{

private:
	uint64_t process_handle_;
};

}
}
}

#endif


