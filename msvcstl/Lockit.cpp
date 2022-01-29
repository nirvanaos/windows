#include <Nirvana/Nirvana.h>
#include <yvals.h>

std::_Lockit::_Lockit () noexcept :
	_Locktype (0)
{}

std::_Lockit::_Lockit (int type) noexcept :
	_Locktype (type)
{}

std::_Lockit::~_Lockit () noexcept
{}

