// Nirvana project.
// Windows implementation.
// ThreadPool template class.

#ifndef NIRVANA_CORE_WINDOWS_THREADPOOL_H_
#define NIRVANA_CORE_WINDOWS_THREADPOOL_H_

#include <Heap.h>
#include "../Port/SystemInfo.h"

namespace Nirvana {
namespace Core {
namespace Windows {

/// Template class for thread pool controlled by a master object.
/// \tparam Master Thread controller class.
/// \tparam Worker Thread class.
/// Worker constructor gets reference to Master as parameter.
/// Worker class must have method Worker::create(int priority);
template <class Master, class Worker>
class ThreadPool :
	public Master
{
	typedef CoreAllocator <Worker> Allocator;

public:
	static unsigned int thread_count ()
	{
		return Port::g_system_info.hardware_concurrency ();
	}

	ThreadPool () :
		Master (),
		threads_ (nullptr)
	{
		Allocator allocator;
		threads_ = allocator.allocate (thread_count ());
		Worker* p = threads_;
		try {
			for (Worker* end = p + thread_count (); p != end; ++p) {
				new (p) Worker (std::ref (static_cast <Master&> (*this)));
			}
		} catch (...) {
			while (p != threads_) {
				(--p)->~Worker ();
			}
			allocator.deallocate (threads_, thread_count ());
			throw;
		}
	}

	~ThreadPool ()
	{
		Allocator allocator;
		for (Worker* p = threads_, *end = p + thread_count (); p != end; ++p) {
			p->~Worker ();
		}
		allocator.deallocate (threads_, thread_count ());
	}

	Worker* threads ()
	{
		return threads_;
	}

	//! Create and start threads.
	void start (int priority)
	{
		Master::start ();
		for (Worker* p = threads_, *end = p + thread_count (); p != end; ++p) {
			p->create (p, priority);
		}
	}

	//! Terminate threads.
	void terminate ()
	{
		Master::terminate ();
		for (Worker* p = threads_, *end = p + thread_count (); p != end; ++p) {
			p->port ().join ();
		}
	}

private:
	Worker* threads_;
};

}
}
}

#endif
