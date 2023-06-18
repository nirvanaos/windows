// Testing Windows API memory management functions
#define PSAPI_VERSION 2
#include <gtest/gtest.h>
#include <Windows.h>
#include <Psapi.h>
#include <string>
#include <set>
#include <winternl.h>
#include "OtherProcess.h"

#define PAGE_SIZE 4096
#define ALLOCATION_GRANULARITY (16 * PAGE_SIZE)

using namespace std;

namespace TestAPI {

class TestAPI :
	public ::testing::Test
{
protected:
	TestAPI ()
	{}

	virtual ~TestAPI ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}

	// Create new mapping
	static HANDLE new_mapping ()
	{
		return CreateFileMappingW (INVALID_HANDLE_VALUE, nullptr, PAGE_EXECUTE_READWRITE | SEC_RESERVE, 0, ALLOCATION_GRANULARITY, nullptr);
	}

	// Duplicate mapping
	static HANDLE dup_mapping (HANDLE mh) {
		HANDLE process = GetCurrentProcess ();
		HANDLE mh1 = nullptr;
		EXPECT_TRUE (DuplicateHandle (process, mh, process, &mh1, 0, FALSE, DUPLICATE_SAME_ACCESS));
		return mh1;
	}

	static BOOL handles_equal (HANDLE h0, HANDLE h1)
	{
		return CompareObjectHandles (h0, h1);
	}

	static DWORD protection (const void* p)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery (p, &mbi, sizeof (mbi)))
			return mbi.Protect;
		else
			return 0;
	}

	static const DWORD MASK_RW = PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_READWRITE | PAGE_WRITECOPY;
	static const DWORD MASK_RO = PAGE_EXECUTE_READ | PAGE_READONLY;
	static const DWORD MASK_ACCESS = MASK_RW | MASK_RO;
};

TEST_F (TestAPI, MappingHandle)
{
	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	HANDLE mh1 = dup_mapping (mh);
	EXPECT_TRUE (handles_equal (mh, mh1));
	EXPECT_TRUE (CloseHandle (mh1));
	EXPECT_TRUE (CloseHandle (mh));
}

TEST_F (TestAPI, Allocate)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	EXPECT_EQ (si.dwAllocationGranularity, ALLOCATION_GRANULARITY);
	EXPECT_EQ (((size_t)si.lpMaximumApplicationAddress + 1) % ALLOCATION_GRANULARITY, 0);

	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	char* p = (char*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (p);

	// Commit 2 pages.
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE * 2, MEM_COMMIT, PAGE_READWRITE));

	EXPECT_FALSE (VirtualAlloc (p, ALLOCATION_GRANULARITY, MEM_RESERVE, PAGE_READWRITE));

	DWORD err = GetLastError ();
	EXPECT_EQ (err, ERROR_INVALID_ADDRESS);

	// Write to 2 pages.
	EXPECT_NO_THROW (p [0] = 1);
	EXPECT_NO_THROW (p [PAGE_SIZE] = 2);

	// Decommit first page. We can't use VirtualFree with mapped memory.

	DWORD old;
	EXPECT_TRUE (VirtualProtect (p, PAGE_SIZE, PAGE_NOACCESS, &old));
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_RESET, PAGE_NOACCESS));

	EXPECT_FALSE (protection (p) & MASK_RW);

	// Recommit first page
	char x;
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	EXPECT_NO_THROW (x = p [0]);

	EXPECT_TRUE (UnmapViewOfFile (p));
	EXPECT_TRUE (CloseHandle (mh));
}

TEST_F (TestAPI, Sharing)
{
	MEMORY_BASIC_INFORMATION mbi0, mbi1;

	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	char* p = (char*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (p);
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));

	EXPECT_TRUE (VirtualQuery (p, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (p + PAGE_SIZE, &mbi1, sizeof (mbi1)));
	EXPECT_EQ (mbi0.AllocationBase, p);
	EXPECT_EQ (mbi1.AllocationBase, p);
	EXPECT_EQ (mbi0.Type, MEM_MAPPED);
	EXPECT_EQ (mbi1.Type, MEM_MAPPED);
	EXPECT_EQ (mbi0.AllocationProtect, PAGE_READWRITE);
	EXPECT_EQ (mbi1.AllocationProtect, PAGE_READWRITE);

	EXPECT_EQ (mbi0.State, MEM_COMMIT);
	EXPECT_EQ (mbi1.State, MEM_RESERVE);
	EXPECT_EQ (mbi0.Protect, PAGE_READWRITE);
	EXPECT_EQ (mbi1.Protect, 0);

	strcpy (p, "test");

	char* copies [10];
	HANDLE handles [10];

	memset (copies, 0, sizeof (copies));
	memset (handles, 0, sizeof (copies));

	DWORD old;
	EXPECT_TRUE (VirtualProtect (p, PAGE_SIZE, PAGE_WRITECOPY, &old));

	EXPECT_TRUE (VirtualQuery (p, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (p + PAGE_SIZE, &mbi1, sizeof (mbi1)));

	// Make copies and check
	for (size_t i = 0; i < size (copies); ++i) {
		HANDLE mh1 = dup_mapping (mh);
		handles [i] = mh1;
		char* p = (char*)MapViewOfFile (mh1, FILE_MAP_READ, 0, 0, ALLOCATION_GRANULARITY);
		EXPECT_TRUE (p);
		copies [i] = p;
		if (p) {
			EXPECT_STREQ (p, "test");
			EXPECT_FALSE (protection (p) & MASK_RW);
		}
	}

	// Release copies
	for (size_t i = 0; i < size (copies); ++i) {
		char* p = copies [i];
		copies [i] = 0;
		if (p)
			EXPECT_TRUE (UnmapViewOfFile (p));
		HANDLE h = handles [i];
		handles [i] = 0;
		if (h)
			EXPECT_TRUE (CloseHandle (h));
	}

	// Make copies and check
	for (size_t i = 0; i < size (copies); ++i) {
		HANDLE mh1 = dup_mapping (mh);
		handles [i] = mh1;
		char* p = (char*)MapViewOfFile (mh1, FILE_MAP_COPY, 0, 0, ALLOCATION_GRANULARITY);
		EXPECT_TRUE (p);
		copies [i] = p;
		if (p) {
			EXPECT_STREQ (p, "test");

			char buf [16];
			_itoa ((int)i, buf, 10);
			EXPECT_NO_THROW (strcpy (p + 4, buf));
		}
	}

	// Change copies
	for (size_t i = 0; i < size (copies); ++i) {
		char* p = copies [i];
		char buf [16] = "test";
		_itoa ((int)i, buf + 4, 10);
		EXPECT_STREQ (p, buf);
	}

	// Check source
	EXPECT_STREQ (p, "test");

	// Release copies
	for (size_t i = 0; i < size (copies); ++i) {
		char* p = copies [i];
		copies [i] = 0;
		if (p)
			EXPECT_TRUE (UnmapViewOfFile (p));
		HANDLE h = handles [i];
		handles [i] = 0;
		if (h)
			EXPECT_TRUE (CloseHandle (h));
	}

	// Make copies and check
	for (size_t i = 0; i < size (copies); ++i) {
		HANDLE mh1 = dup_mapping (mh);
		handles [i] = mh1;
		char* p = (char*)MapViewOfFile (mh1, FILE_MAP_COPY, 0, 0, ALLOCATION_GRANULARITY);
		EXPECT_TRUE (p);
		copies [i] = p;
		EXPECT_STREQ (p, "test");
	}

	// Commit one more page
	EXPECT_TRUE (VirtualAlloc (p + PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	strcpy (p + PAGE_SIZE, "test1");

	// Decommit source page
	EXPECT_TRUE (VirtualProtect (p, PAGE_SIZE, PAGE_NOACCESS | PAGE_REVERT_TO_FILE_MAP, &old));
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_RESET, PAGE_NOACCESS));

	// Check copies
	for (size_t i = 0; i < size (copies); ++i) {
		char* p = copies [i];
		EXPECT_STREQ (p, "test");
	}

	// Release source
	EXPECT_TRUE (UnmapViewOfFile (p));
	EXPECT_TRUE (CloseHandle (mh));
}

TEST_F (TestAPI, Protection)
{
	MEMORY_BASIC_INFORMATION mbi0, mbi1;
	DWORD old;

	// We use "execute" protection to distinct mapped pages from write-copied pages.
	// Page states:
	// 0 - Not committed.
	// PAGE_NOACCESS - Decommitted (mapped, but not accessible).
	// PAGE_EXECUTE_READWRITE: The page is mapped and never was shared.
	// PAGE_WRITECOPY: The page is mapped and was shared.
	// PAGE_READWRITE: The page is write-copyed (private, disconnected from mapping).
	// PAGE_EXECUTE_READONLY: The read-only mapped page never was shared.
	// PAGE_EXECUTE: The read-only mapped page was shared.
	// PAGE_READONLY: The page is not mapped. Page was write-copyed, than access was changed from PAGE_READWRITE to PAGE_READONLY.
	// Note: "Page was shared" means that page has been shared at least once. Currently, page may be still shared or already not.

	// Page state changes.
	// Prepare to share:
	//   0 (not committed)->PAGE_NOACCESS (commit+decommit)
	//   PAGE_EXECUTE_READWRITE->PAGE_WRITECOPY
	//   PAGE_EXECUTE_READONLY, PAGE_WRITECOPY, PAGE_EXECUTE, PAGE_NOACCESS unchanged.
	//   PAGE_READWRITE, PAGE_READONLY - we need to remap the block.
	// Remap:
	//   PAGE_READWRITE->PAGE_EXECUTE_READWRITE
	//   PAGE_READONLY->PAGE_EXECUTE_READONLY
	// Write-protect:
	//   PAGE_EXECUTE_READWRITE<->PAGE_EXECUTE_READONLY
	//	 PAGE_WRITECOPY<->PAGE_EXECUTE
	//   PAGE_READWRITE<->PAGE_READONLY

	// Create source block
	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	BYTE* src = (BYTE*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (src);

	// Commit 2 pages
	EXPECT_TRUE (VirtualAlloc (src, PAGE_SIZE * 2, MEM_COMMIT, PAGE_EXECUTE_READWRITE));
	src [0] = 1;
	src [PAGE_SIZE] = 2;

	EXPECT_TRUE (VirtualQuery (src, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (src + PAGE_SIZE * 2, &mbi1, sizeof (mbi1)));
	EXPECT_EQ (mbi0.AllocationBase, src);
	EXPECT_EQ (mbi1.AllocationBase, src);
	EXPECT_EQ (mbi0.Type, MEM_MAPPED);
	EXPECT_EQ (mbi1.Type, MEM_MAPPED);
	EXPECT_EQ (mbi0.AllocationProtect, PAGE_EXECUTE_READWRITE);
	EXPECT_EQ (mbi1.AllocationProtect, PAGE_EXECUTE_READWRITE);

	EXPECT_EQ (mbi0.State, MEM_COMMIT);
	EXPECT_EQ (mbi1.State, MEM_RESERVE);
	EXPECT_EQ (mbi0.Protect, PAGE_EXECUTE_READWRITE);
	EXPECT_EQ (mbi1.Protect, 0);

	// Virtual copy first page to target block
	
	// 1. Change committed data protection to write-copy
	EXPECT_TRUE (VirtualProtect (src, PAGE_SIZE, PAGE_WRITECOPY, &old));

	// 2. Map to destination
	BYTE* dst = (BYTE*)MapViewOfFile (mh, FILE_MAP_COPY, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (dst);

	// 3. Protect non-copyed data
	EXPECT_TRUE (VirtualProtect (dst + PAGE_SIZE, PAGE_SIZE, PAGE_NOACCESS, &old));

	EXPECT_TRUE (VirtualQuery (dst, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (dst + PAGE_SIZE, &mbi1, sizeof (mbi1)));
	EXPECT_EQ (mbi0.AllocationBase, dst);
	EXPECT_EQ (mbi1.AllocationBase, dst);
	EXPECT_EQ (mbi0.Type, MEM_MAPPED);
	EXPECT_EQ (mbi1.Type, MEM_MAPPED);
	EXPECT_EQ (mbi0.AllocationProtect, PAGE_WRITECOPY);
	EXPECT_EQ (mbi1.AllocationProtect, PAGE_WRITECOPY);

	EXPECT_EQ (mbi0.State, MEM_COMMIT);
	EXPECT_EQ (mbi1.State, MEM_COMMIT);
	EXPECT_EQ (mbi0.Protect, PAGE_WRITECOPY);
	EXPECT_EQ (mbi1.Protect, PAGE_NOACCESS);

	EXPECT_EQ (dst [0], src [0]);
	// Write to destination
	dst [0] = 3;
	EXPECT_EQ (src [0], 1);	// Source not changed

	EXPECT_TRUE (VirtualQuery (dst, &mbi0, sizeof (mbi0)));
	EXPECT_EQ (mbi0.Protect, PAGE_READWRITE); // Private write-copyed

	// Copy source again
	//
	// PAGE_REVERT_TO_FILE_MAP can be combined with other protection
	// values to specify to VirtualProtect that the argument range
	// should be reverted to point back to the backing file.  This
	// means the contents of any private (copy on write) pages in the
	// range will be discarded.  Any reverted pages that were locked
	// into the working set are unlocked as well.
	//
	EXPECT_TRUE (VirtualProtect (dst, PAGE_SIZE, PAGE_WRITECOPY | PAGE_REVERT_TO_FILE_MAP, &old));
	EXPECT_EQ (dst [0], src [0]);

	// Write to source (copy-on-write)
	src [1] = 4;

	// Check source state
	EXPECT_TRUE (VirtualQuery (src, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (src + PAGE_SIZE, &mbi1, sizeof (mbi1)));

	EXPECT_EQ (mbi0.Protect, PAGE_READWRITE); // Private write-copyed
	EXPECT_EQ (mbi1.Protect, PAGE_EXECUTE_READWRITE); // Shared read-write

	EXPECT_EQ (dst [0], 1);	// Destination not changed

	// Decommit source page
	EXPECT_TRUE (VirtualProtect (src, PAGE_SIZE, PAGE_NOACCESS | PAGE_REVERT_TO_FILE_MAP, &old));
	EXPECT_FALSE (protection (src) & MASK_ACCESS);

	EXPECT_EQ (dst [0], 1);	// Destination not changed

	// Decommit second source page
	EXPECT_TRUE (VirtualProtect (src + PAGE_SIZE, PAGE_SIZE, PAGE_NOACCESS, &old));
	// For not shared pages call VirtualAlloc with MEM_RESET to inform system that page content is not more interested.
	EXPECT_TRUE (VirtualAlloc (src + PAGE_SIZE, PAGE_SIZE, MEM_RESET, PAGE_NOACCESS));
	EXPECT_FALSE (protection (src + PAGE_SIZE) & MASK_ACCESS);

	EXPECT_TRUE (UnmapViewOfFile (dst));
	EXPECT_TRUE (UnmapViewOfFile (src));
	EXPECT_TRUE (CloseHandle (mh));
}

// SparseMapping on 64 bit system over 10 times faster than SharedMapping
TEST_F (TestAPI, SparseMapping)
{
	WCHAR dir [MAX_PATH + 1];
	ASSERT_TRUE (GetCurrentDirectoryW ((DWORD)size (dir), dir));

	HANDLE file = CreateFileW (L"mapping.tmp", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
		CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
	ASSERT_NE (file, INVALID_HANDLE_VALUE);

	{
		FILE_SET_SPARSE_BUFFER sb;
		sb.SetSparse = TRUE;
		DWORD cb;
		EXPECT_TRUE (DeviceIoControl (file, FSCTL_SET_SPARSE, &sb, sizeof (sb), 0, 0, &cb, 0));
	}

	HANDLE mapping;
	HANDLE* table;
	{
		SYSTEM_INFO si;
		GetSystemInfo (&si);
		size_t size = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY * sizeof (HANDLE);

		FILE_ZERO_DATA_INFORMATION zdi;
		zdi.FileOffset.QuadPart = 0;
		zdi.BeyondFinalZero.QuadPart = size;
		DWORD cb;
		EXPECT_TRUE (DeviceIoControl (file, FSCTL_SET_ZERO_DATA, &zdi, sizeof (zdi), 0, 0, &cb, 0));
		mapping = CreateFileMapping (file, 0, PAGE_READWRITE, zdi.BeyondFinalZero.HighPart, zdi.BeyondFinalZero.LowPart, 0);
		EXPECT_TRUE (mapping);
	}
	table = (HANDLE*)MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	EXPECT_TRUE (table);

	HANDLE sfile = CreateFileW (L"mapping.tmp", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
	EXPECT_NE (sfile, INVALID_HANDLE_VALUE);

	LARGE_INTEGER fsize;
	EXPECT_TRUE (GetFileSizeEx (sfile, &fsize));

	HANDLE smapping = CreateFileMapping (sfile, 0, PAGE_READWRITE, fsize.HighPart, fsize.LowPart, 0);
	EXPECT_TRUE (smapping);
	HANDLE* stable = (HANDLE*)MapViewOfFile (smapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	table [ALLOCATION_GRANULARITY / sizeof (HANDLE)] = INVALID_HANDLE_VALUE;
	EXPECT_EQ (stable [ALLOCATION_GRANULARITY / sizeof (HANDLE)], INVALID_HANDLE_VALUE);

	EXPECT_TRUE (UnmapViewOfFile (stable));
	EXPECT_TRUE (CloseHandle (smapping));
	EXPECT_TRUE (CloseHandle (sfile));

	EXPECT_TRUE (UnmapViewOfFile (table));
	EXPECT_TRUE (CloseHandle (mapping));
	EXPECT_TRUE (CloseHandle (file));
}

// Single level directory implementation for 32-bit system.
TEST_F (TestAPI, SharedMapping)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	LARGE_INTEGER size;
	size.QuadPart = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY * sizeof (HANDLE);

	HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, L"NirvanaMapping");
	ASSERT_TRUE (mapping);
	HANDLE* table = (HANDLE*)MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, 0, 0, (SIZE_T)size.QuadPart);
	EXPECT_TRUE (table);

	HANDLE smapping = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, L"NirvanaMapping");
	ASSERT_TRUE (smapping);

	HANDLE* stable = (HANDLE*)MapViewOfFile (smapping, FILE_MAP_ALL_ACCESS, 0, 0, (SIZE_T)size.QuadPart);
	EXPECT_TRUE (stable);

	EXPECT_TRUE (VirtualAlloc (table + ALLOCATION_GRANULARITY / sizeof (HANDLE), PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	table [ALLOCATION_GRANULARITY / sizeof (HANDLE)] = INVALID_HANDLE_VALUE;

	EXPECT_TRUE (VirtualAlloc (stable + ALLOCATION_GRANULARITY / sizeof (HANDLE), PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	EXPECT_EQ (stable [ALLOCATION_GRANULARITY / sizeof (HANDLE)], INVALID_HANDLE_VALUE);

	EXPECT_TRUE (UnmapViewOfFile (stable));
	EXPECT_TRUE (CloseHandle (smapping));

	EXPECT_TRUE (UnmapViewOfFile (table));
	EXPECT_TRUE (CloseHandle (mapping));
}

// Two level directory implementation for 64-bit system.
TEST_F (TestAPI, SharedMapping2)
{
	SYSTEM_INFO si;
	GetSystemInfo (&si);
	LARGE_INTEGER size;
	size.QuadPart = ((size_t)si.lpMaximumApplicationAddress + ALLOCATION_GRANULARITY) / ALLOCATION_GRANULARITY * sizeof (HANDLE);

	HANDLE mapping = CreateFileMappingW (INVALID_HANDLE_VALUE, 0, PAGE_READWRITE | SEC_RESERVE, size.HighPart, size.LowPart, L"NirvanaMapping");
	ASSERT_TRUE (mapping);

	static const size_t SECOND_LEVEL_SIZE = ALLOCATION_GRANULARITY / sizeof (HANDLE);

	size_t root_size = ((size_t)size.QuadPart + ALLOCATION_GRANULARITY - 1) / ALLOCATION_GRANULARITY;

	HANDLE** root_dir = (HANDLE**)VirtualAlloc (0, root_size, MEM_RESERVE, PAGE_READWRITE);
	ASSERT_TRUE (root_dir);

	size_t idx = ALLOCATION_GRANULARITY / sizeof (HANDLE);
	size_t i1 = idx / SECOND_LEVEL_SIZE;
	size_t i2 = idx % SECOND_LEVEL_SIZE;

	ASSERT_TRUE (VirtualAlloc (root_dir + i1, sizeof (HANDLE**), MEM_COMMIT, PAGE_READWRITE));

	LARGE_INTEGER offset;
	offset.QuadPart = ALLOCATION_GRANULARITY * i1;
	EXPECT_TRUE (root_dir [i1] = (HANDLE*)MapViewOfFile (mapping, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, ALLOCATION_GRANULARITY));
	ASSERT_TRUE (VirtualAlloc (root_dir[i1] + i2, sizeof (HANDLE), MEM_COMMIT, PAGE_READWRITE));

	HANDLE smapping = OpenFileMappingW (FILE_MAP_ALL_ACCESS, FALSE, L"NirvanaMapping");
	ASSERT_TRUE (smapping);

	HANDLE** sroot_dir = (HANDLE**)VirtualAlloc (0, root_size, MEM_RESERVE, PAGE_READWRITE);
	ASSERT_TRUE (sroot_dir);

	ASSERT_TRUE (VirtualAlloc (sroot_dir + i1, sizeof (HANDLE**), MEM_COMMIT, PAGE_READWRITE));

	EXPECT_TRUE (sroot_dir [i1] = (HANDLE*)MapViewOfFile (smapping, FILE_MAP_ALL_ACCESS, offset.HighPart, offset.LowPart, ALLOCATION_GRANULARITY));
	ASSERT_TRUE (VirtualAlloc (sroot_dir [i1] + i2, sizeof (HANDLE), MEM_COMMIT, PAGE_READWRITE));

	root_dir [i1][i2] = INVALID_HANDLE_VALUE;
	EXPECT_EQ (sroot_dir [i1][i2], INVALID_HANDLE_VALUE);

	EXPECT_TRUE (UnmapViewOfFile (sroot_dir [i1]));
	EXPECT_TRUE (CloseHandle (smapping));

	EXPECT_TRUE (UnmapViewOfFile (root_dir [i1]));
	EXPECT_TRUE (CloseHandle (mapping));
}

TEST_F (TestAPI, Commit)
{
	// Create source block
	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	BYTE* src = (BYTE*)MapViewOfFile (mh, FILE_MAP_ALL_ACCESS | FILE_MAP_EXECUTE, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (src);

	// Commit 2 pages overlapped
	EXPECT_TRUE (VirtualAlloc (src, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READ));
	EXPECT_TRUE (VirtualAlloc (src + PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_EXECUTE_READWRITE));

	MEMORY_BASIC_INFORMATION mbi0, mbi1;
	EXPECT_TRUE (VirtualQuery (src, &mbi0, sizeof (mbi0)));
	EXPECT_TRUE (VirtualQuery (src + PAGE_SIZE, &mbi1, sizeof (mbi1)));
	EXPECT_EQ (mbi0.Protect, PAGE_EXECUTE_READ);
	EXPECT_EQ (mbi1.Protect, PAGE_EXECUTE_READWRITE);

	EXPECT_TRUE (UnmapViewOfFile (src));
	EXPECT_TRUE (CloseHandle (mh));
}

TEST_F (TestAPI, Placeholder)
{
	HANDLE process = GetCurrentProcess ();
	MEMORY_BASIC_INFORMATION mbi;

	// Reserve 3 regions
	uint8_t* placeholder = (uint8_t*)VirtualAlloc2 (process, nullptr, 3 * ALLOCATION_GRANULARITY, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
	ASSERT_TRUE (placeholder);

	EXPECT_EQ (VirtualQuery (placeholder, &mbi, sizeof (mbi)), sizeof (mbi));
	EXPECT_EQ (mbi.AllocationBase, placeholder);
	EXPECT_EQ (mbi.RegionSize, 3 * ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.State, MEM_RESERVE);
	EXPECT_EQ (mbi.Protect, 0);
	EXPECT_EQ (mbi.Type, MEM_PRIVATE);

	// Split to 3 regions
	EXPECT_TRUE (VirtualFreeEx (process, placeholder + ALLOCATION_GRANULARITY, ALLOCATION_GRANULARITY, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER));

	EXPECT_EQ (VirtualQuery (placeholder, &mbi, sizeof (mbi)), sizeof (mbi));
	EXPECT_EQ (mbi.AllocationBase, placeholder);
	EXPECT_EQ (mbi.RegionSize, ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.State, MEM_RESERVE);
	EXPECT_EQ (mbi.Protect, 0);
	EXPECT_EQ (mbi.Type, MEM_PRIVATE);

	EXPECT_EQ (VirtualQuery (placeholder + ALLOCATION_GRANULARITY, &mbi, sizeof (mbi)), sizeof (mbi));
	EXPECT_EQ (mbi.AllocationBase, placeholder + ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.RegionSize, ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.State, MEM_RESERVE);
	EXPECT_EQ (mbi.Protect, 0);
	EXPECT_EQ (mbi.Type, MEM_PRIVATE);

	EXPECT_EQ (VirtualQuery (placeholder + 2 * ALLOCATION_GRANULARITY, &mbi, sizeof (mbi)), sizeof (mbi));
	EXPECT_EQ (mbi.AllocationBase, placeholder + 2 * ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.RegionSize, ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.State, MEM_RESERVE);
	EXPECT_EQ (mbi.Protect, 0);
	EXPECT_EQ (mbi.Type, MEM_PRIVATE);

	EXPECT_FALSE (VirtualAlloc2 (process, placeholder + ALLOCATION_GRANULARITY, ALLOCATION_GRANULARITY, MEM_RESERVE, PAGE_NOACCESS, nullptr, 0));

	static const ULONG PROTECTION = PAGE_EXECUTE_READWRITE;

	// Map middle region
	HANDLE mh = new_mapping ();
	EXPECT_EQ (MapViewOfFile3 (mh, process, placeholder + ALLOCATION_GRANULARITY, 0, ALLOCATION_GRANULARITY, 
		MEM_REPLACE_PLACEHOLDER, PROTECTION, nullptr, 0), placeholder + ALLOCATION_GRANULARITY);

	EXPECT_EQ (VirtualQuery (placeholder + ALLOCATION_GRANULARITY, &mbi, sizeof (mbi)), sizeof (mbi));
	EXPECT_EQ (mbi.AllocationBase, placeholder + ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.RegionSize, ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.State, MEM_RESERVE);
	EXPECT_EQ (mbi.Protect, 0);
	EXPECT_EQ (mbi.Type, MEM_MAPPED);

	// Commit
	EXPECT_EQ (VirtualAlloc2 (process, placeholder + ALLOCATION_GRANULARITY, PAGE_SIZE, MEM_COMMIT,
		PROTECTION, nullptr, 0), placeholder + ALLOCATION_GRANULARITY);

	EXPECT_EQ (VirtualQuery (placeholder + ALLOCATION_GRANULARITY, &mbi, sizeof (mbi)), sizeof (mbi));
	EXPECT_EQ (mbi.AllocationBase, placeholder + ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.RegionSize, PAGE_SIZE);
	EXPECT_EQ (mbi.State, MEM_COMMIT);
	EXPECT_EQ (mbi.Protect, PROTECTION);
	EXPECT_EQ (mbi.Type, MEM_MAPPED);

	placeholder [ALLOCATION_GRANULARITY] = 1;

	// Remap
	EXPECT_TRUE (UnmapViewOfFile2 (process, placeholder + ALLOCATION_GRANULARITY, MEM_PRESERVE_PLACEHOLDER));
	CloseHandle (mh);

	EXPECT_EQ (VirtualQuery (placeholder + ALLOCATION_GRANULARITY, &mbi, sizeof (mbi)), sizeof (mbi));
	EXPECT_EQ (mbi.AllocationBase, placeholder + ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.RegionSize, ALLOCATION_GRANULARITY);
	EXPECT_EQ (mbi.State, MEM_RESERVE);
	EXPECT_EQ (mbi.Protect, 0);
	EXPECT_EQ (mbi.Type, MEM_PRIVATE);

	mh = new_mapping ();
	EXPECT_EQ (MapViewOfFile3 (mh, process, placeholder + ALLOCATION_GRANULARITY, 0, ALLOCATION_GRANULARITY,
		MEM_REPLACE_PLACEHOLDER, PROTECTION, nullptr, 0), placeholder + ALLOCATION_GRANULARITY);

	// Release
	EXPECT_TRUE (VirtualFreeEx (process, placeholder, 0, MEM_RELEASE));
	EXPECT_TRUE (UnmapViewOfFile2 (process, placeholder + ALLOCATION_GRANULARITY, 0));
	CloseHandle (mh);
	EXPECT_TRUE (VirtualFreeEx (process, placeholder + 2 * ALLOCATION_GRANULARITY, 0, MEM_RELEASE));
}

class WorkingSet
{
public:
	WorkingSet () :
		wsi_cb_ (sizeof (PSAPI_WORKING_SET_INFORMATION)),
		wsi_ ((PSAPI_WORKING_SET_INFORMATION*)malloc (sizeof (PSAPI_WORKING_SET_INFORMATION)))
	{
		wsi_->NumberOfEntries = 0;
	}

	~WorkingSet ()
	{
		free (wsi_);
	}

	void query ()
	{
		while (!QueryWorkingSet (GetCurrentProcess (), wsi_, wsi_cb_)) {
			ASSERT_EQ (ERROR_BAD_LENGTH, GetLastError ());
			DWORD cb = (DWORD)(sizeof (PSAPI_WORKING_SET_INFORMATION) + sizeof (PSAPI_WORKING_SET_BLOCK) * (wsi_->NumberOfEntries - 1));
			free (wsi_);
			wsi_ = nullptr;
			wsi_ = (PSAPI_WORKING_SET_INFORMATION*)malloc (cb);
			wsi_cb_ = cb;
		}
	}

	const PSAPI_WORKING_SET_BLOCK* begin () const
	{
		return wsi_->WorkingSetInfo;
	}

	const PSAPI_WORKING_SET_BLOCK* end () const
	{
		return wsi_->WorkingSetInfo + wsi_->NumberOfEntries;
	}

	size_t unique_pages_cnt () const
	{
		set <size_t> pages;
		for (auto p = begin (); p != end (); ++p) {
			pages.insert (p->VirtualPage);
		}
		return pages.size ();
	}

private:
	DWORD wsi_cb_;
	PSAPI_WORKING_SET_INFORMATION* wsi_;
};

TEST_F (TestAPI, ZeroedPage)
{
	// Try to detect if Windows support zeroed page COW.
	static const size_t PAGE_COUNT = 32;

	PROCESS_MEMORY_COUNTERS pmc [4];
	for (auto p = begin (pmc); p != end (pmc); ++p)
		p->cb = sizeof (*pmc);

	GetProcessMemoryInfo (GetCurrentProcess (), pmc + 0, sizeof (*pmc));

	PSAPI_WORKING_SET_EX_INFORMATION page_info [PAGE_COUNT];

	int* mem = (int*)VirtualAlloc (nullptr, PAGE_COUNT * PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	ASSERT_TRUE (mem);

	GetProcessMemoryInfo (GetCurrentProcess (), pmc + 1, sizeof (*pmc));

	for (size_t i = 0; i < PAGE_COUNT; ++i) {
		page_info [i].VirtualAddress = mem + i * PAGE_SIZE / sizeof (int);
	}

	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), page_info, sizeof (page_info)));

	for (size_t i = 0; i < PAGE_COUNT; ++i) {
		EXPECT_EQ (page_info [i].VirtualAttributes.Valid, 0);
	}

	for (const int* p = mem, *end = mem + PAGE_COUNT * PAGE_SIZE / sizeof (int); p != end; ++p) {
		EXPECT_EQ (*p, 0);
	}

	GetProcessMemoryInfo (GetCurrentProcess (), pmc + 2, sizeof (*pmc));

	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), page_info, sizeof (page_info)));

	for (size_t i = 0; i < PAGE_COUNT; ++i) {
		// Page state changes after read.
		EXPECT_EQ (page_info [i].VirtualAttributes.Valid, 1);
	}

	for (int* p = mem, *end = mem + PAGE_COUNT * PAGE_SIZE / sizeof (int); p != end; ++p) {
		*p = 1;
	}

	PSAPI_WORKING_SET_EX_INFORMATION page_info_w [PAGE_COUNT];
	for (size_t i = 0; i < PAGE_COUNT; ++i) {
		page_info_w [i].VirtualAddress = mem + i * PAGE_SIZE / sizeof (int);
	}

	GetProcessMemoryInfo (GetCurrentProcess (), pmc + 3, sizeof (*pmc));

	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), page_info_w, sizeof (page_info_w)));

	// Page state does not change after write.
	EXPECT_EQ (memcmp (page_info, page_info_w, sizeof (page_info)), 0);

	// CONCLUSION: Windows does not use zero page COW unlike Linux.
	VirtualFree (mem, 0, MEM_RELEASE);
}

TEST_F (TestAPI, PageState)
{
	PSAPI_WORKING_SET_EX_INFORMATION info;

	// Reserve section
	HANDLE mh = new_mapping ();
	ASSERT_TRUE (mh);
	void* mem = MapViewOfFile (mh, FILE_MAP_ALL_ACCESS, 0, 0, ALLOCATION_GRANULARITY);
	ASSERT_TRUE (mem);
	info.VirtualAddress = mem;
	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), &info, sizeof (info)));
	EXPECT_FALSE (info.VirtualAttributes.Valid);
	EXPECT_FALSE (info.VirtualAttributes.Shared);
	// Win32Protection is invalid but always zero
	EXPECT_EQ (info.VirtualAttributes.Win32Protection, 0);

	// Commit page private
	EXPECT_TRUE (VirtualAlloc (mem, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), &info, sizeof (info)));
	// If committed page was not accessed, it remains invalid.
	EXPECT_FALSE (info.VirtualAttributes.Valid);
	EXPECT_TRUE (info.VirtualAttributes.Shared);

	MEMORY_BASIC_INFORMATION mbi;
	EXPECT_EQ (VirtualQuery (mem, &mbi, sizeof (mbi)), sizeof (mbi));
	EXPECT_EQ (mbi.Protect, PAGE_READWRITE);

	// VirtualQuery does not change page state
	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), &info, sizeof (info)));
	// If committed page was not accessed, it remains invalid.
	EXPECT_FALSE (info.VirtualAttributes.Valid);

	// Read from private page
	EXPECT_EQ (*(int*)mem, 0);
	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), &info, sizeof (info)));
	EXPECT_TRUE (info.VirtualAttributes.Valid);
	EXPECT_TRUE (info.VirtualAttributes.Shared);
	EXPECT_EQ (info.VirtualAttributes.Win32Protection, PAGE_READWRITE);

	// Write to private page
	*(int*)mem = 1;
	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), &info, sizeof (info)));
	EXPECT_TRUE (info.VirtualAttributes.Valid);
	EXPECT_TRUE (info.VirtualAttributes.Shared);
	EXPECT_EQ (info.VirtualAttributes.Win32Protection, PAGE_READWRITE);

	// Share page
	DWORD old;
	EXPECT_TRUE (VirtualProtect (mem, PAGE_SIZE, PAGE_WRITECOPY, &old));
	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), &info, sizeof (info)));
	EXPECT_TRUE (info.VirtualAttributes.Valid);
	EXPECT_TRUE (info.VirtualAttributes.Shared);
	EXPECT_EQ (*(int*)mem, 1);
	EXPECT_EQ (info.VirtualAttributes.Win32Protection, PAGE_WRITECOPY);

	// Write to shared page
	*(int*)mem = 1;
	ASSERT_TRUE (QueryWorkingSetEx (GetCurrentProcess (), &info, sizeof (info)));
	EXPECT_TRUE (info.VirtualAttributes.Valid);
	EXPECT_FALSE (info.VirtualAttributes.Shared);
	EXPECT_EQ (info.VirtualAttributes.Win32Protection, PAGE_READWRITE);

	UnmapViewOfFile (mem);
	CloseHandle (mh);
}

TEST_F (TestAPI, HandleSharing)
{
	HANDLE mh = new_mapping ();
	PUBLIC_OBJECT_BASIC_INFORMATION info;
	ASSERT_FALSE (NtQueryObject (mh, ObjectBasicInformation, &info, sizeof (info), nullptr));
	EXPECT_EQ (info.HandleCount, 1);
	HANDLE mh1 = dup_mapping (mh);
	ASSERT_FALSE (NtQueryObject (mh, ObjectBasicInformation, &info, sizeof (info), nullptr));
	EXPECT_EQ (info.HandleCount, 2);
	ASSERT_FALSE (NtQueryObject (mh1, ObjectBasicInformation, &info, sizeof (info), nullptr));
	EXPECT_EQ (info.HandleCount, 2);

	CloseHandle (mh);
	CloseHandle (mh1);
}

TEST_F (TestAPI, Mailslot)
{
	static const WCHAR name [] = L"\\\\.\\mailslot\\Nirvana\\TestAPI";
	HANDLE ms_read = CreateMailslotW (name, sizeof (int), MAILSLOT_WAIT_FOREVER, nullptr);
	ASSERT_TRUE (ms_read);
	HANDLE ms_write = CreateFileW (name, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	EXPECT_TRUE (ms_write);

	int msg = 1;
	DWORD cb;
	EXPECT_TRUE (WriteFile (ms_write, &msg, sizeof (msg), &cb, nullptr));
	EXPECT_TRUE (ReadFile (ms_read, &msg, sizeof (msg), &cb, nullptr));

	CloseHandle (ms_read);
	EXPECT_FALSE (WriteFile (ms_write, &msg, sizeof (msg), &cb, nullptr));

	CloseHandle (ms_write);
}

TEST_F (TestAPI, Semaphore)
{
	HANDLE hsem = CreateSemaphoreW (nullptr, 0, 2, nullptr);
	ASSERT_TRUE (hsem);
	
	EXPECT_EQ (WaitForSingleObject (hsem, 0), WAIT_TIMEOUT);

	wstring cmd = L"ReleaseSemaphore.exe ";
	cmd += to_wstring (GetCurrentProcessId ());
	cmd += L' ';
	cmd += to_wstring ((uintptr_t)hsem);

	STARTUPINFOW si;
	memset (&si, 0, sizeof (si));
	si.cb = sizeof (si);
	PROCESS_INFORMATION pi;
	BOOL ok = CreateProcessW (nullptr, (WCHAR*)cmd.c_str (), nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
	ASSERT_TRUE (ok);
	EXPECT_TRUE (CloseHandle (pi.hThread));

	WaitForSingleObject (pi.hProcess, INFINITE);

	DWORD ret;
	EXPECT_TRUE (GetExitCodeProcess (pi.hProcess, &ret));
	EXPECT_EQ (ret, 0);
	CloseHandle (pi.hProcess);

	EXPECT_EQ (WaitForSingleObject (hsem, 0), WAIT_OBJECT_0);

	CloseHandle (hsem);
}

const NT_TIB* get_TIB ()
{
	return (const NT_TIB*)NtCurrentTeb ();
}

long CALLBACK exception_filter (struct _EXCEPTION_POINTERS* pex)
{
	if (EXCEPTION_ACCESS_VIOLATION == pex->ExceptionRecord->ExceptionCode)
		throw runtime_error ("Test");
	return EXCEPTION_CONTINUE_SEARCH;
}

extern void test_exception (int* p);

TEST_F (TestAPI, Exception)
{
	void* h = AddVectoredExceptionHandler (TRUE, &exception_filter);
	bool ok = false;
	try {
		test_exception (nullptr);
	} catch (...) {
		ok = true;
	}
	RemoveVectoredExceptionHandler (h);
	EXPECT_TRUE (ok);
}

TEST_F (TestAPI, OtherProcess)
{
	// Create mapping
	HANDLE hm = new_mapping ();
	//HANDLE hm = CreateFileMapping2 (INVALID_HANDLE_VALUE, nullptr, FILE_MAP_ALL_ACCESS, PAGE_EXECUTE_READWRITE, SEC_RESERVE,
	//	ALLOCATION_GRANULARITY, nullptr, nullptr, 0);
	ASSERT_TRUE (hm);
	char* p = (char*)MapViewOfFile (hm, FILE_MAP_ALL_ACCESS, 0, 0, ALLOCATION_GRANULARITY);
	EXPECT_TRUE (p);

	// Commit 1 page.
	EXPECT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	if (p)
		strcpy (p, "Test");

	// Create other process
	wstring cmd = L"OtherProcess.exe";

	STARTUPINFOW si;
	memset (&si, 0, sizeof (si));
	si.cb = sizeof (si);
	PROCESS_INFORMATION pi;
	BOOL ok = CreateProcessW (nullptr, (WCHAR*)cmd.c_str (), nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
	ASSERT_TRUE (ok);
	EXPECT_TRUE (CloseHandle (pi.hThread));
	HANDLE mailslot;
	for (int i = 0; i < 3; ++i) {
		mailslot = CreateFileW (TEST_MAILSLOT_NAME, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (INVALID_HANDLE_VALUE == mailslot)
			Sleep (1000);
		else
			break;
	}
	EXPECT_NE (INVALID_HANDLE_VALUE, mailslot);

	HANDLE process = OpenProcess (PROCESS_QUERY_INFORMATION
		| PROCESS_VM_OPERATION | PROCESS_DUP_HANDLE, FALSE, pi.dwProcessId);
	ASSERT_TRUE (process);

	// Duplicate and map
	OtherProcessMsg msg;
	msg.mapping = nullptr;
	msg.address = nullptr;
	EXPECT_TRUE (DuplicateHandle (GetCurrentProcess (), hm, process, &msg.mapping, FILE_MAP_ALL_ACCESS, TRUE, DUPLICATE_SAME_ACCESS));

	// We have to map source handle hm, which belongs to the current process, not msg.mapping which belongs to the target process.
	if (msg.mapping)
		msg.address = (char*)MapViewOfFile3 (hm, process, nullptr, 0, ALLOCATION_GRANULARITY, 0, PAGE_EXECUTE_READWRITE, nullptr, 0);
	EXPECT_TRUE (msg.address);
	EXPECT_TRUE (UnmapViewOfFile (p));
	EXPECT_TRUE (CloseHandle (hm));
	EXPECT_TRUE (CloseHandle (process));

	DWORD cbw;
	if (msg.address)
		EXPECT_TRUE (WriteFile (mailslot, &msg, sizeof (msg), &cbw, nullptr));
	EXPECT_TRUE (WriteFile (mailslot, &msg, 1, &cbw, nullptr));
	CloseHandle (mailslot);

	WaitForSingleObject (pi.hProcess, INFINITE);
	DWORD ec = -1;
	EXPECT_TRUE (GetExitCodeProcess (pi.hProcess, &ec));
	CloseHandle (pi.hProcess);
	EXPECT_EQ (ec, 0);
}

TEST_F (TestAPI, CommitProblem)
{
	HANDLE hm = new_mapping ();
	void* p = MapViewOfFile3 (hm, GetCurrentProcess (), nullptr, 0, ALLOCATION_GRANULARITY, 0,
		PAGE_READWRITE, nullptr, 0);
	ASSERT_TRUE (p);
	ASSERT_TRUE (VirtualAlloc (p, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	DWORD old;
	ASSERT_TRUE (VirtualProtect (p, PAGE_SIZE, PAGE_WRITECOPY, &old));
	HANDLE hm1 = dup_mapping (hm);
	void* p1 = MapViewOfFile3 (hm1, GetCurrentProcess (), nullptr, 0, ALLOCATION_GRANULARITY, 0,
		PAGE_WRITECOPY, nullptr, 0);
	// We can not commit PAGE_READWRITE if mapping is PAGE_WRITECOPY
	EXPECT_FALSE (VirtualAlloc ((BYTE*)p1 + PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	UnmapViewOfFile (p1);
	CloseHandle (hm1);

	hm1 = dup_mapping (hm);
	p1 = MapViewOfFile3 (hm1, GetCurrentProcess (), nullptr, 0, ALLOCATION_GRANULARITY, 0,
		PAGE_READONLY, nullptr, 0);
	// We can not commit PAGE_READWRITE if mapping is PAGE_READONLY
	EXPECT_FALSE (VirtualAlloc ((BYTE*)p1 + PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	UnmapViewOfFile (p1);
	CloseHandle (hm1);

	hm1 = dup_mapping (hm);
	p1 = MapViewOfFile3 (hm1, GetCurrentProcess (), nullptr, 0, ALLOCATION_GRANULARITY, 0,
		PAGE_READWRITE, nullptr, 0);
	ASSERT_TRUE (VirtualProtect (p1, PAGE_SIZE, PAGE_WRITECOPY, &old));
	EXPECT_TRUE (VirtualAlloc ((BYTE*)p1 + PAGE_SIZE, PAGE_SIZE, MEM_COMMIT, PAGE_READWRITE));
	UnmapViewOfFile (p1);
	CloseHandle (hm1);

	UnmapViewOfFile (p);
	CloseHandle (hm);
}

TEST_F (TestAPI, FinalName)
{
	WCHAR path [MAX_PATH + 1];
	GetModuleFileNameW (nullptr, path, (DWORD)std::size (path));
	HANDLE h = CreateFileW (path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	ASSERT_NE (h, INVALID_HANDLE_VALUE);

	WCHAR fn [MAX_PATH + 1];
	EXPECT_TRUE (GetFinalPathNameByHandleW (h, fn, (DWORD)std::size (fn), 0));
	CloseHandle (h);

	*wcsrchr (path, L'\\') = L'\0';
	h = CreateFileW (path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	ASSERT_NE (h, INVALID_HANDLE_VALUE);

	EXPECT_TRUE (GetFinalPathNameByHandleW (h, fn, (DWORD)std::size (fn), 0));
	CloseHandle (h);
}

}
