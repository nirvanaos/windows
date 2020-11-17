// Nirvana project.
// Windows implementation.
// System domain (computer).

#ifndef NIRVANA_CORE_PORT_SYSDOMAIN_H_
#define NIRVANA_CORE_PORT_SYSDOMAIN_H_

#include "../Source/Mailslot.h"

namespace Nirvana {
namespace Core {
namespace Port {

class SysDomain
{
public:
	class ProtDomainInfo
	{
	public:
		struct ProcessStart;
		ProtDomainInfo (int platform); // Will invoke CreateProcess().
		ProtDomainInfo (const ProcessStart& message);

		ProtDomainInfo (ProtDomainInfo&& src) :
			process_id_ (src.process_id_),
			process_ (src.process_),
			semaphore_ (src.semaphore_),
			mailslot_ (std::move (src.mailslot_))
		{
			src.process_ = nullptr;
			src.semaphore_ = nullptr;
		}

		void process_start ();

		~ProtDomainInfo ();

		uint32_t domain_id () const
		{
			return process_id_;
		}

	private:
		void create_semaphore ();

	private:
		uint32_t process_id_;
		void* process_;
		void* semaphore_;
		Windows::Mailslot mailslot_;
	};
};

}
}
}

#endif
