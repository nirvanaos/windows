#ifndef NIRVANA_CORE_WINDOWS_CONSOLE_H_
#define NIRVANA_CORE_WINDOWS_CONSOLE_H_

#include <core.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class Console
{
public:
	Console ();
	~Console ();

	const Console& operator << (const char* text) const NIRVANA_NOEXCEPT;
	
	const Console& operator << (char c) const NIRVANA_NOEXCEPT
	{
		write (&c, 1);
		return *this;
	}

private:
	void write (const char* text, size_t len) const NIRVANA_NOEXCEPT;

private:
	void* handle_;
};

}
}
}

#endif
