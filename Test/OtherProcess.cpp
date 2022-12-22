#include <windows.h>

int main (int argc, char** argv)
{
	HANDLE event = OpenEventW (SYNCHRONIZE, FALSE, L"WindowsTestOtherDomain");
	if (!event)
		return -1;
	WaitForSingleObject (event, INFINITE);
	return 0;
}
