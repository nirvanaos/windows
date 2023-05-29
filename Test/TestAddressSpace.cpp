// Test for other domain communications.
#include <Nirvana/Nirvana.h>
#include "../Port/Memory.h"
#include "../Source/OtherSpace.inl"
#include "../Source/MailslotName.h"
#include "../Source/Mailslot.h"
#include <gtest/gtest.h>

using namespace Nirvana::Core::Port;
using namespace Nirvana::Core::Windows;
using namespace ESIOP::Windows;

struct Message
{
	uint64_t address, size;
};

int other_process ()
{
  HANDLE mailslot = CreateMailslotW (MailslotName (GetCurrentProcessId ()),
    8, MAILSLOT_WAIT_FOREVER, nullptr);
  if (INVALID_HANDLE_VALUE == mailslot)
    return -1;

	Memory::initialize ();

	Message msg;
	for (;;) {
		DWORD cb_read = 0;
		if (!ReadFile (mailslot, &msg, sizeof (msg), &cb_read, nullptr))
			return -1;
		if (cb_read == sizeof (msg)) {
			if (!msg.address)
				break;
			Memory::release ((void*)msg.address, (size_t)msg.size);
		} else
			break;
	}
	CloseHandle (mailslot);

	Memory::terminate ();

	return 0;
}

class TestAddressSpace :
	public ::testing::Test
{
protected:
	TestAddressSpace () :
		other_process_id_ (0),
		other_process_handle_ (nullptr)
	{
	}

	virtual ~TestAddressSpace ()
	{
	}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		ASSERT_TRUE (Nirvana::Core::Port::Memory::initialize ());
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		if (other_process_handle_) {
			Mailslot mailslot;
			mailslot.open (MailslotName (other_process_id_));
			Message msg{ 0, 0 };
			mailslot.send (msg);
			WaitForSingleObject (other_process_handle_, INFINITE);
			CloseHandle (other_process_handle_);
		}
		Nirvana::Core::Port::Memory::terminate ();
	}

	void start_other_process (const WCHAR* platform)
	{
		std::wstring cmd;
		{
			WCHAR path [MAX_PATH + 1];
			size_t len = GetModuleFileNameW (nullptr, path, (DWORD)std::size (path));

			WCHAR* config_end = find_slash (path, path + len);
			std::wstring name (config_end + 1, path + len);
			WCHAR* platform_end = find_slash (path, config_end);
			std::wstring config (platform_end + 1, config_end);
			WCHAR* sol_end = find_slash (path, platform_end);
			WCHAR* end = std::copy (platform, platform + wcslen (platform), sol_end + 1);
			*(end++) = L'\\';
			end = std::copy (config.begin (), config.end (), end);
			*(end++) = L'\\';
			end = std::copy (name.begin (), name.end (), end);

			cmd += L'\"';
			cmd.append ((const WCHAR*)path, end);
			cmd += L"\" o";
		}

		STARTUPINFOW si;
		memset (&si, 0, sizeof (si));
		si.cb = sizeof (si);
		PROCESS_INFORMATION pi;
		BOOL ok = CreateProcessW (nullptr, (WCHAR*)cmd.data (), nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
		ASSERT_TRUE (ok);
		EXPECT_TRUE (CloseHandle (pi.hThread));
		other_process_id_ = pi.dwProcessId;
		other_process_handle_ = pi.hProcess;
	}

	static WCHAR* find_slash (WCHAR* begin, WCHAR* end)
	{
		while (end > begin) {
			if (L'\\' == *--end)
				return end;
		}
		return begin;
	}

	DWORD other_process_id () const
	{
		return other_process_id_;
	}

	HANDLE other_process_handle () const
	{
		return other_process_handle_;
	}

private:
	DWORD other_process_id_;
	HANDLE other_process_handle_;
};

TEST_F (TestAddressSpace, ReserveCopy64)
{
	start_other_process (L"x64");
	OtherSpace <true> space (other_process_id (), other_process_handle ());

	size_t block_size = 0x10000;
	size_t cb = block_size;
	void* block = Memory::allocate (nullptr, cb, 0);
	ESIOP::SharedMemPtr p = space.reserve (cb);
	EXPECT_EQ (p, space.copy (p, block, cb, true));
	space.release (p, cb);
//	Memory::release (block, cb);
}

TEST_F (TestAddressSpace, Copy64)
{
	start_other_process (L"x64");
	OtherSpace <true> space (other_process_id (), other_process_handle ());

	size_t block_size = 0x10000;
	size_t cb = block_size;
	void* block = Memory::allocate (nullptr, cb, 0);
	ESIOP::SharedMemPtr p = space.copy (0, block, cb, false);
	space.release (p, cb);
	Memory::release (block, cb);
}

int main (int argc, char** argv)
{
  if (argc > 1 && !strcmp (argv [1], "o")) {
    return other_process ();
  } else {
		other_space_init ();
    testing::InitGoogleTest (&argc, argv);
    return RUN_ALL_TESTS ();
  }
}

