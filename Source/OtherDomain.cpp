#include "../Port/OtherDomain.h"
#include "OtherSpace.inl"
#include "MailslotName.h"

namespace ESIOP {

namespace Windows {

Mailslot::Mailslot (ProtDomainId domain_id)
{
	open (Nirvana::Core::Windows::MailslotName (domain_id));
}

}

#ifdef NIRVANA_SINGLE_PLATFORM

OtherDomain::OtherDomain (ProtDomainId domain_id) :
	Windows::Mailslot (domain_id),
	Space (domain_id, ::OpenProcess (PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, domain_id))
{}

SharedMemPtr OtherDomain::reserve (size_t size)
{
	return Space::reserve (size);
}

SharedMemPtr OtherDomain::copy (SharedMemPtr reserved, void* src, size_t& size, bool release_src)
{
	return Space::copy (reserved, src, size, release_src);
}

void OtherDomain::release (SharedMemPtr p, size_t size)
{
	Space::release (p, size);
}

#else

namespace Windows {

template <bool x64>
class OtherDomainImpl :
	public OtherDomain,
	private OtherSpace <x64>
{
	typedef OtherSpace <x64> Space;

public:
	OtherDomainImpl (ProtDomainId process_id, HANDLE process_handle) :
		Space (process_id, process_handle)
	{}

	virtual SharedMemPtr reserve (size_t size) override
	{
		return Space::reserve (size);
	}

	virtual SharedMemPtr copy (SharedMemPtr reserved, void* src, size_t& size, bool release_src) override
	{
		return Space::copy (reserved, src, size, release_src);
	}

	virtual void release (SharedMemPtr p, size_t size) override
	{
		Space::release (p, size);
	}

	virtual void get_sizes (PlatformSizes& sizes) NIRVANA_NOEXCEPT override
	{
		Space::get_sizes (sizes);
	}

	virtual void* store_pointer (void* where, SharedMemPtr p) NIRVANA_NOEXCEPT override
	{
		return Space::store_pointer (where, p);
	}

	virtual void* store_size (void* where, size_t size) NIRVANA_NOEXCEPT override
	{
		return Space::store_size (where, size);
	}
};

}

OtherDomain::OtherDomain (ProtDomainId domain_id) :
	Windows::Mailslot (domain_id),
	implementation_ (nullptr)
{
	HANDLE process = ::OpenProcess (PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, domain_id);
	bool x64 =
#ifdef _WIN64
		true;
#else
		false;
#endif
	USHORT process_machine;
	if (::IsWow64Process2 (process, &process_machine, nullptr) && IMAGE_FILE_MACHINE_I386 == process_machine)
		x64 = false;
	if (x64)
		implementation_ = new Windows::OtherDomainImpl <true> (domain_id, process);
	else
		implementation_ = new Windows::OtherDomainImpl <false> (domain_id, process);
}

#endif

}
