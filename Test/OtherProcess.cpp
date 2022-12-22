#include "OtherProcess.h"

int main (int argc, char** argv)
{
	HANDLE mailslot = CreateMailslotW (TEST_MAILSLOT_NAME, sizeof (OtherProcessMsg), MAILSLOT_WAIT_FOREVER, nullptr);
	if (INVALID_HANDLE_VALUE == mailslot)
		return -1;

	OtherProcessMsg msg;
	for (;;) {
		DWORD cb_read = 0;
		if (!ReadFile (mailslot, &msg, sizeof (msg), &cb_read, nullptr))
			return -1;
		if (cb_read == sizeof (msg)) {
			if (strcmp (msg.address, "Test"))
				return -1;
			UnmapViewOfFile (msg.address);
			if (!CloseHandle (msg.mapping))
				return -1;
		} else
			break;
	}
	CloseHandle (mailslot);
	return 0;
}
