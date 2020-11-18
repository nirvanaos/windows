#include <windows.h>
#include <stdlib.h>

int main (int argc, char** argv)
{
	if (argc < 3)
		return -1;

	char* end;

	DWORD pid = strtoul (argv [1], &end, 10);
	if (*end)
		return -1;

	HANDLE hproc = OpenProcess (PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hproc)
		return GetLastError ();

	HANDLE hsem = (HANDLE)(uintptr_t)strtoul (argv [2], &end, 10);
	if (*end)
		return -1;

	HANDLE hsem1;

	if (!DuplicateHandle (hproc, hsem, GetCurrentProcess (), &hsem1, 0, FALSE, DUPLICATE_SAME_ACCESS))
		return GetLastError ();

	if (!ReleaseSemaphore (hsem1, 1, nullptr))
		return GetLastError ();

	CloseHandle (hsem1);

	return 0;
}
