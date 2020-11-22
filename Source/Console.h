#ifndef NIRVANA_CORE_WINDOWS_CONSOLE_H_
#define NIRVANA_CORE_WINDOWS_CONSOLE_H_

namespace Nirvana {
namespace Core {
namespace Windows {

class Console
{
public:
	static void write (const char* text);

private:
	static void* handle_;
};

}
}
}

#endif
