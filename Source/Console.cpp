#include "Console.h"
#include "win32.h"
#include <string.h>

namespace Nirvana {
namespace Core {
namespace Windows {

Console::Console () :
	handle_ (nullptr)
{
	AllocConsole ();
	handle_ = GetStdHandle (STD_ERROR_HANDLE);
}

Console::~Console ()
{
	if (handle_) {
		operator << ("Press any key to close this window...\n");
		HANDLE h = GetStdHandle (STD_INPUT_HANDLE);
		INPUT_RECORD input;
		DWORD cnt;
		while (ReadConsoleInput (h, &input, 1, &cnt)) {
			if (KEY_EVENT == input.EventType && input.Event.KeyEvent.uChar.UnicodeChar)
				break;
		}
	}
}

const Console& Console::operator << (const char* text) const NIRVANA_NOEXCEPT
{
	write (text, strlen (text));
	return *this;
}

void Console::write (const char* text, size_t len) const NIRVANA_NOEXCEPT
{
	if (handle_) {
		DWORD cb;
		WriteFile (handle_, text, (DWORD)len, &cb, nullptr);
	}
}

}
}
}
