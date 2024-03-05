#include "../Port/Security.h"
#include "../Source/WinWChar.h"
#include "../Source/SecurityInfo.h"
#include "../Source/Handle.h"
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
		Nirvana::Core::Port::Security::initialize ();
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
	unsigned mode = si.get_mode ();
	EXPECT_EQ (mode & S_IRWXU, S_IRWXU);
}

}

