#include "../Source/MessageBroker.h"
#include "../Port/Chrono.h"
#include "../Port/Timer.h"
#include <gtest/gtest.h>

namespace TestTimer {

using namespace Nirvana::Core;
using namespace Nirvana::Core::Windows;
using namespace Nirvana::Core::Port;

class TestTimer :
	public ::testing::Test
{
protected:
	TestTimer ()
	{}

	virtual ~TestTimer ()
	{}

	// If the constructor and destructor are not enough for setting up
	// and cleaning up each test, you can define the following methods:

	virtual void SetUp ()
	{
		// Code here will be called immediately after the constructor (right
		// before each test).
		Nirvana::Core::Port::SystemInfo::initialize ();
		Nirvana::Core::Port::Chrono::initialize ();
		ASSERT_TRUE (Nirvana::Core::Heap::initialize ());
		MessageBroker::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		MessageBroker::terminate ();
		Nirvana::Core::Heap::terminate ();
		Nirvana::Core::Port::Chrono::terminate ();
	}
};

class TimerTest :
	public Timer
{
protected:
	TimerTest ()
	{
		signalled_ = false;
		destructed_ = false;
	}

	~TimerTest ()
	{
		destructed_ = true;
	}

	virtual void signal () NIRVANA_NOEXCEPT
	{
		signalled_ = true;
	}

public:
	static bool signalled_;
	static bool destructed_;
};

bool TimerTest::signalled_ = false;
bool TimerTest::destructed_ = false;

TEST_F (TestTimer, Set)
{
	{
		Ref <TimerTest> t = Ref <TimerTest>::create <ImplDynamic <TimerTest> > ();
		t->set (0, 1 * TimeBase::SECOND, 0);

		Sleep (3000);
	}
	EXPECT_TRUE (TimerTest::signalled_);
	EXPECT_TRUE (TimerTest::destructed_);
}

}
