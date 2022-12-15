#include "../Port/OtherDomain.h"
#include "OtherSpace.inl"
#include "MailslotName.h"

using namespace Nirvana::Core::Windows;

namespace ESIOP {
namespace Windows {

OtherDomainBase::OtherDomainBase (ProtDomainId domain_id) :
	process_ (::OpenProcess (PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, domain_id))
{
	if (!process_ || !open (Nirvana::Core::Windows::MailslotName (domain_id)))
		Nirvana::throw_COMM_FAILURE ();
}

OtherDomainBase::~OtherDomainBase ()
{
	if (process_)
		CloseHandle (process_);
}

void OtherDomainBase::initialize ()
{
#if !defined (_WIN64) && !defined (NIRVANA_SINGLE_PLATFORM)
	DWORD64 ntdll = GetModuleHandle64 (L"ntdll.dll");
	wow64_NtQueryVirtualMemory = GetProcAddress64 (ntdll, "NtQueryVirtualMemory");
	wow64_NtProtectVirtualMemory = GetProcAddress64 (ntdll, "NtProtectVirtualMemory");
	wow64_NtAllocateVirtualMemoryEx = GetProcAddress64 (ntdll, "NtAllocateVirtualMemoryEx");
	wow64_NtFreeVirtualMemory = GetProcAddress64 (ntdll, "NtFreeVirtualMemory");
	wow64_NtMapViewOfSectionEx = GetProcAddress64 (ntdll, "NtMapViewOfSectionEx");
	wow64_NtUnmapViewOfSectionEx = GetProcAddress64 (ntdll, "NtUnmapViewOfSectionEx");
#endif
}

inline bool OtherDomainBase::is_64_bit () const NIRVANA_NOEXCEPT
{
	bool x64 =
#ifdef _WIN64
	true;
#else
	false;
#endif
	USHORT process_machine;
	if (::IsWow64Process2 (process_, &process_machine, nullptr) && IMAGE_FILE_MACHINE_I386 == process_machine)
		x64 = false;
	return x64;
}

}

#ifdef NIRVANA_SINGLE_PLATFORM

OtherDomain::OtherDomain (ProtDomainId domain_id) :
	Windows::OtherDomainBase (domain_id),
	Space (domain_id, process ())
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
	Windows::OtherDomainBase (domain_id),
	implementation_ (nullptr)
{
	if (is_64_bit ())
		implementation_ = new Windows::OtherDomainImpl <true> (domain_id, process ());
	else
		implementation_ = new Windows::OtherDomainImpl <false> (domain_id, process ());
}

#endif

}
