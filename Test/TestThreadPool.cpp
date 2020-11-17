#include <core.h>
#include "../Source/PostOffice.h"
#include "../Source/Mailslot.h"
#include "../Source/MailslotName.h"
#include "../Source/ThreadPostman.h"
#include <gtest/gtest.h>
#include <atomic>

namespace TestThreadPool {

using namespace ::std;
using namespace ::Nirvana::Core::Windows;

struct Message
{
	LONG_PTR tag;
};

const unsigned TAG = 123456789;

class PostOffice :
	public ::Nirvana::Core::Windows::PostOffice <PostOffice, sizeof (Message), THREAD_PRIORITY_NORMAL>
{
	typedef ::Nirvana::Core::Windows::PostOffice <PostOffice, sizeof (Message), THREAD_PRIORITY_NORMAL> Base;
public:
	PostOffice ()
	{
		Base::create_mailslot (MailslotName (GetCurrentProcessId ()));
		Base::start ();
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
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
	}
};

TEST_F (TestThreadPool, PostOffice)
{
	Mailslot mailslot;
	EXPECT_FALSE (mailslot.open (MailslotName (GetCurrentProcessId ())));
	PostOffice po;
	EXPECT_TRUE (mailslot.open (MailslotName (GetCurrentProcessId ())));

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
