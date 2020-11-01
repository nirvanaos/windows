// Nirvana project.
// Windows implementation.
// System domain (computer).

#ifndef NIRVANA_CORE_PORT_SYSDOMAIN_H_
#define NIRVANA_CORE_PORT_SYSDOMAIN_H_

#include "../Source/SchedulerIPC.h"
#include "../Source/Mailslot.h"

namespace Nirvana {
namespace Core {
namespace Port {

class SysDomain
{
public:
	class ProtDomainInfo :
		private Windows::SchedulerIPC
	{
	public:
		ProtDomainInfo (uint32_t process_id) :
			process_id_ (process_id)
		{}

		uint32_t process_id () const
		{
			return process_id_;
		}

		void process_start (uint32_t id)
		{
			if (process_id_ == id) {
				static const wchar_t prefix [] = EXECUTE_MAILSLOT_PREFIX;
				try {
					execute_mailslot_.open (prefix, id);
					ProcessStartAck msg = {0};
					execute_mailslot_.send (msg);
					return;
				} catch (...) {
				}
			}
			// TODO: Kill process by id.
		}

		void execute (const Execute& msg) NIRVANA_NOEXCEPT
		{
			try {
				execute_mailslot_.send (msg);
			} catch (...) {
				// TODO: Process is dead, remove process info.
			}
		}

	private:
		Windows::Mailslot execute_mailslot_;
		uint32_t process_id_;
	};
};

}
}
}

#endif
