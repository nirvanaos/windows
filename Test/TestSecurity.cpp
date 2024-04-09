#include "../Port/Security.h"
#include "../Source/WinWChar.h"
#include "../Source/SecurityInfo.h"
#include "../Source/Handle.h"
#include "../Source/FileSecurityAttributes.h"
#include <gtest/gtest.h>

using namespace Nirvana::Core;

namespace TestSecurity {

class TestSecurity :
	public ::testing::Test
{
protected:
	TestSecurity ()
	{}

	virtual ~TestSecurity ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		ASSERT_TRUE (Nirvana::Core::Port::Security::initialize ());
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Nirvana::Core::Port::Security::terminate ();
	}
};

TEST_F (TestSecurity, File)
{
	Windows::WinWChar path [MAX_PATH];
	ASSERT_LT (GetModuleFileNameW (nullptr, path, (DWORD)std::size (path)), std::size (path));

	Windows::Handle handle = CreateFileW (path, READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	ASSERT_TRUE (handle.is_valid ());

	Windows::SecurityInfoDirItem si (handle);

	Nirvana::SecurityId owner_id = Port::Security::make_security_id (si.owner ());
	Nirvana::SecurityId group_id = Port::Security::make_security_id (si.group ());
	IDL::String owner = Port::Security::get_name (owner_id);
	IDL::String group = Port::Security::get_name (group_id);

	unsigned access = si.get_access (Port::Security::prot_domain_context ());
	EXPECT_EQ (access, F_OK | R_OK | W_OK | X_OK);
}

TEST_F (TestSecurity, CreateFile)
{
	Windows::WinWChar tmp_path [MAX_PATH];
	ASSERT_LT (GetTempPathW ((DWORD)std::size (tmp_path), tmp_path), (DWORD)std::size (tmp_path));
	Windows::WinWChar tmp_name [MAX_PATH];

	struct Mode
	{
		unsigned requested, expected;
	};

	static const Mode modes [] = {
		{ 0, S_IRWXU },
		{ S_IRWXU, S_IRWXU },
		{ S_IRUSR | S_IWUSR, S_IRUSR | S_IWUSR },
		{ S_IRUSR | S_IWUSR | S_IROTH, S_IRUSR | S_IWUSR | S_IROTH },
		{ S_IRUSR | S_IWUSR | S_IRGRP, S_IRUSR | S_IWUSR | S_IRGRP }
	};

	for (const Mode& mode : modes) {
		ASSERT_NE (GetTempFileNameW (tmp_path, L"tst", 0, tmp_name), 0);

		DeleteFileW (tmp_name);

		Windows::FileSecurityAttributes sa (Port::Security::prot_domain_context (), mode.requested, false);

		Windows::Handle handle = CreateFileW (tmp_name, GENERIC_READ | GENERIC_WRITE, 0,
			sa.security_attributes (), CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);

		ASSERT_TRUE (handle.is_valid ());

		Windows::SecurityInfoDirItem si (handle);

		unsigned mode1 = si.get_mode ();
		Nirvana::SecurityId owner_id = Port::Security::make_security_id (si.owner ());
		Nirvana::SecurityId group_id = Port::Security::make_security_id (si.group ());
		IDL::String owner = Port::Security::get_name (owner_id);
		IDL::String group = Port::Security::get_name (group_id);

		handle.close ();
		DeleteFileW (tmp_name);

		unsigned expected_mode = mode.expected;
		if (group_id.empty ())
			expected_mode &= ~S_IRWXG;

		EXPECT_EQ (mode1, expected_mode);
	}
}

}

