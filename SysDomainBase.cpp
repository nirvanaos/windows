// Nirvana project.
// Windows implementation.
// System domain (computer).

#include "../SysDomain.h"

namespace Nirvana {
namespace Core {
namespace Windows {

SysDomain* SysDomainBase::singleton_ = nullptr;

void SysDomainBase::initialize ()
{
	singleton_ = new SysDomain;
}

void SysDomainBase::terminate ()
{
	delete singleton_;
	singleton_ = nullptr;
}

}
}
}