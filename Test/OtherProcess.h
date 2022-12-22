#pragma once
#include <Windows.h>

struct OtherProcessMsg
{
	HANDLE mapping;
	char* address;
};

#define TEST_MAILSLOT_NAME L"\\\\.\\mailslot\\WindowsTest\\OtherProcess"
