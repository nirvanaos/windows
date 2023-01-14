/// \file
/*
* Nirvana Core. Windows port library.
*
* This is a part of the Nirvana project.
*
* Author: Igor Popov
*
* Copyright (c) 2021 Igor Popov.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*
* Send comments and/or bug reports to:
*  popov.nirvana@gmail.com
*/
#ifndef NIRVANA_CORE_WINDOWS_THREADPOOL_H_
#define NIRVANA_CORE_WINDOWS_THREADPOOL_H_
#pragma once

#include <SharedAllocator.h>
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
	typedef SharedAllocator <Worker> Allocator;

public:
	static unsigned int thread_count () NIRVANA_NOEXCEPT
	{
		return Master::thread_count ();
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

	Worker* threads () NIRVANA_NOEXCEPT
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
	void terminate () NIRVANA_NOEXCEPT;

private:
	Worker* threads_;
};

template <class Master, class Worker>
void ThreadPool <Master, Worker>::terminate () NIRVANA_NOEXCEPT
{
	Master::terminate ();
	for (Worker* p = threads_, *end = p + thread_count (); p != end; ++p) {
		p->join ();
	}
}

}
}
}

#endif
