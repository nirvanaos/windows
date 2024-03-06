#include "../Port/Security.h"
#include "../Source/WinWChar.h"
#include "../Source/SecurityInfo.h"
#include "../Source/Handle.h"
#include "../Source/error2errno.h"
#include <gtest/gtest.h>

using namespace Nirvana::Core;

namespace Nirvana {
namespace Core {
namespace Windows {

class FileSecurityAttributes
{
public:
	FileSecurityAttributes (unsigned mode, DWORD inheritance = NO_INHERITANCE);
	~FileSecurityAttributes ();

	SECURITY_ATTRIBUTES* security_attributes () const noexcept
	{
		return psa_;
	}

private:
	SECURITY_ATTRIBUTES* psa_;
	SECURITY_ATTRIBUTES sa_;
};

FileSecurityAttributes::FileSecurityAttributes (unsigned mode, DWORD inheritance) :
	psa_ (nullptr)
{
	zero (sa_);

	if (mode) {

		if (!(mode & S_IRWXU))
			mode |= S_IRWXU;

		EXPLICIT_ACCESS_W ea [3];
		zero (ea);

		if (mode & S_IRUSR)
			ea [0].grfAccessPermissions |= FILE_READ_DATA;
		if (mode & S_IWUSR)
			ea [0].grfAccessPermissions |= FILE_WRITE_DATA;
		if (mode & S_IXUSR)
			ea [0].grfAccessPermissions |= FILE_EXECUTE;
		ea [0].grfAccessMode = SET_ACCESS;
		ea [0].grfInheritance = inheritance;
		ea [0].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea [0].Trustee.ptstrName = (LPWCH)Port::Security::creator_owner ();
		EXPLICIT_ACCESS_W* ea_end = ea + 1;

		if (mode & S_IRWXG) {
			if (mode & S_IRGRP)
				ea_end->grfAccessPermissions |= FILE_READ_DATA;
			if (mode & S_IWGRP)
				ea_end->grfAccessPermissions |= FILE_WRITE_DATA;
			if (mode & S_IXGRP)
				ea_end->grfAccessPermissions |= FILE_EXECUTE;
			ea_end->grfAccessMode = SET_ACCESS;
			ea_end->grfInheritance = inheritance;
			ea_end->Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			ea_end->Trustee.ptstrName = (LPWCH)Port::Security::creator_group ();
			++ea_end;
		}

		if (mode & S_IRWXO) {
			if (mode & S_IROTH)
				ea_end->grfAccessPermissions |= FILE_READ_DATA;
			if (mode & S_IWOTH)
				ea_end->grfAccessPermissions |= FILE_WRITE_DATA;
			if (mode & S_IXOTH)
				ea_end->grfAccessPermissions |= FILE_EXECUTE;
			ea_end->grfAccessMode = SET_ACCESS;
			ea_end->grfInheritance = inheritance;
			ea_end->Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
			ea_end->Trustee.ptstrName = (LPWCH)Port::Security::everyone ();
			++ea_end;
		}

		// Create a new ACL that contains the new ACEs.
		PACL acl = nullptr;
		DWORD err = SetEntriesInAclW ((ULONG)(ea_end - ea), ea, nullptr, &acl);
		if (err)
			Windows::throw_win_error_sys (err);

		try {

			// Initialize a security descriptor.
			size_t cb = SECURITY_DESCRIPTOR_MIN_LENGTH;
			sa_.lpSecurityDescriptor = (SECURITY_DESCRIPTOR*)memory->allocate (0, cb, Memory::ZERO_INIT);

			if (!InitializeSecurityDescriptor (sa_.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
				Windows::throw_last_error ();

			// Add the ACL to the security descriptor.
			if (!SetSecurityDescriptorDacl (sa_.lpSecurityDescriptor, true, acl, false))
				Windows::throw_last_error ();

			// Initialize a security attributes structure.
			sa_.nLength = sizeof (SECURITY_ATTRIBUTES);
			psa_ = &sa_;

		} catch (...) {
			if (acl)
				LocalFree (acl);
			if (sa_.lpSecurityDescriptor)
				memory->release (sa_.lpSecurityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);
			throw;
		}

		LocalFree (acl);
	}
}

FileSecurityAttributes::~FileSecurityAttributes ()
{
	if (sa_.lpSecurityDescriptor)
		memory->release (sa_.lpSecurityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);
}

}
}
}

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

TEST_F (TestSecurity, CreateFile)
{
	Windows::WinWChar tmp_path [MAX_PATH];
	ASSERT_LT (GetTempPathW ((DWORD)std::size (tmp_path), tmp_path), (DWORD)std::size (tmp_path));
	Windows::WinWChar tmp_name [MAX_PATH];
	ASSERT_NE (GetTempFileNameW (tmp_path, L"tst", 0, tmp_name), 0);

	DeleteFileW (tmp_name);

	unsigned mode0 = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	Windows::FileSecurityAttributes sa (mode0);

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

	EXPECT_EQ (mode1, mode0);
}

}

