#include "Console.h"
#include "win32.h"
#include <string.h>

namespace Nirvana {
namespace Core {
namespace Windows {

void* Console::handle_;

void Console::write (const char* text)
{
	if (!handle_) {
		AllocConsole ();
		handle_ = GetStdHandle (STD_OUTPUT_HANDLE);
		SetConsoleCP (CP_UTF8);
	}
	DWORD cb;
	WriteFile (handle_, text, (DWORD)strlen (text), &cb, nullptr);
}

}
}
}
