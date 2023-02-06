#include "../Source/MessageBroker.h"
#include "../Port/Chrono.h"
#include "../Port/Timer.h"
#include <gtest/gtest.h>
#include <atomic>

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
		Nirvana::Core::Port::Timer::initialize ();
	}

	virtual void TearDown ()
	{
		// Code here will be called immediately after each test (right
		// before the destructor).
		Nirvana::Core::Port::Timer::terminate ();
		Nirvana::Core::Heap::terminate ();
		Nirvana::Core::Port::Chrono::terminate ();
	}
};

class TimerTest :
	public Port::Timer
{
public:
	TimerTest () :
		signalled_ (0)
	{}

	~TimerTest ()
	{}

protected:
	virtual void signal () NIRVANA_NOEXCEPT
	{
		++signalled_;
	}

public:
	std::atomic <unsigned> signalled_;
};

TEST_F (TestTimer, Set)
{
	TimerTest timer;
	timer.set (0, 1 * TimeBase::SECOND, 0);
	Sleep (1500);
	EXPECT_EQ (timer.signalled_, 1);
}

TEST_F (TestTimer, Cancel)
{
	TimerTest timer;
	timer.set (0, 1 * TimeBase::SECOND, 0);
	timer.cancel ();
	Sleep (1500);
	EXPECT_EQ (timer.signalled_, 0);
}

TEST_F (TestTimer, Destruct)
{
	{
		TimerTest timer;
		timer.set (0, 1 * TimeBase::SECOND, 0);
	}
	Sleep (2000);
}

}
