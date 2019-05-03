#include "../Source/PostOffice.h"
#include "../Source/Mailslot.h"
#include "../Source/ThreadPoolable.h"
#include <MockMemory.h>
#include <gtest/gtest.h>
#include <atomic>

namespace TestThreadPool {

using namespace ::std;
using namespace ::Nirvana::Core::Windows;

#define OBJ_NAME_PREFIX L"Nirvana"
const WCHAR mailslot_prefix [] = L"\\\\.\\mailslot\\" OBJ_NAME_PREFIX L"\\unit_test_mailslot";

struct Message
{
	LONG_PTR tag;
};

const unsigned TAG = 123456789;

class PostOffice :
	public ::Nirvana::Core::Windows::PostOffice <PostOffice, sizeof (Message), ThreadPoolable>
{
	typedef ::Nirvana::Core::Windows::PostOffice <PostOffice, sizeof (Message), ThreadPoolable> Base;
public:
	PostOffice ()
	{
		Base::initialize (mailslot_prefix, GetCurrentProcessId ());
	}

	void received (void* message, DWORD size)
	{
		assert (size == sizeof (Message));
		assert (((Message*)message)->tag == TAG);
		++message_cnt_;
	}

	size_t message_cnt () const
	{
		return message_cnt_;
	}

private:
	atomic <size_t> message_cnt_;
};

class TestThreadPool :
	public ::testing::Test
{
protected:
	TestThreadPool ()
	{}

	virtual ~TestThreadPool ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		::Nirvana::Core::Test::mock_memory_init ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		::Nirvana::Core::Test::mock_memory_term ();
	}
};

TEST_F (TestThreadPool, PostOffice)
{
	Mailslot mailslot;
	EXPECT_FALSE (mailslot.open (mailslot_prefix, GetCurrentProcessId ()));
	PostOffice po;
	EXPECT_TRUE (mailslot.open (mailslot_prefix, GetCurrentProcessId ()));

	static const unsigned MESSAGE_CNT = 100;
	for (unsigned i = 0; i < MESSAGE_CNT; ++i) {
		Message msg;
		msg.tag = TAG;
		mailslot.send (msg);
	}
	
	Sleep (3000);
	EXPECT_EQ (po.message_cnt (), MESSAGE_CNT);
}

}
