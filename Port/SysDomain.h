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
		ProtDomainInfo (DWORD process_id) :
			process_id_ (process_id)
		{}

		DWORD process_id () const
		{
			return process_id_;
		}

		void process_start (DWORD id)
		{
			if (process_id_ == id) {
				static const WCHAR prefix [] = EXECUTE_MAILSLOT_PREFIX;
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

		void execute (const Execute& msg)
		{
			execute_mailslot_.send (msg);
		}

	private:
		Windows::Mailslot execute_mailslot_;
		DWORD process_id_;
	};
};

}
}
}

#endif
