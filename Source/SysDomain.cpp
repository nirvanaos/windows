// Nirvana project.
// Windows implementation.
// System domain (computer).

#include "../Port/SysDomain.h"
#include "../Port/SystemInfo.h"
#include "SchedulerMaster.h"
#include "Message.h"
#include "MailslotName.h"

namespace Nirvana {
namespace Core {
namespace Port {

struct SysDomain::ProtDomainInfo::ProcessStart : Windows::Message::ProcessStart
{};

SysDomain::ProtDomainInfo::ProtDomainInfo () :
	process_ (nullptr),
	semaphore_ (nullptr)
{
	create_semaphore ();
	// TODO: Call CreateProcess with parameters "-p <sysdom process id> <semaphore handle>".
	// Initialize process_id_ and process_ with returned values.
}

SysDomain::ProtDomainInfo::ProtDomainInfo (const ProcessStart& message) :
	process_id_ (message.process_id),
	process_ (nullptr),
	semaphore_ (nullptr)
{
}

SysDomain::ProtDomainInfo::~ProtDomainInfo ()
{
	if (process_)
		CloseHandle (process_);
	if (semaphore_) {
		// Release unused processor cores
		while (WAIT_OBJECT_0 == WaitForSingleObject (semaphore_, 0))
			Windows::SchedulerMaster::core_free_static ();
		CloseHandle (semaphore_);
	}
}

void SysDomain::ProtDomainInfo::create_semaphore ()
{
	if (!(semaphore_ = CreateSemaphoreW (nullptr, 0, (LONG)g_system_info.hardware_concurrency (), nullptr)))
		throw_NO_MEMORY ();
}

void SysDomain::ProtDomainInfo::process_start ()
{
	mailslot_.open (Windows::MailslotName (process_id_));
	if (!process_) {
		try {
			if (!(process_ = OpenProcess (SYNCHRONIZE | PROCESS_TERMINATE, FALSE, process_id_)))
				throw_INTERNAL ();
			create_semaphore ();
		} catch (...) {
			try {
				mailslot_.send (Windows::Message::ProcessStartResponse (0, 0));
			} catch (...) {
			}
		}
		mailslot_.send (Windows::Message::ProcessStartResponse (GetCurrentProcessId (), semaphore_));
	}
}

}
}
}
