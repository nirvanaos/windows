// Nirvana project
// Windows implementation.
// PostOffice template.
// Used to receive messages from mailslot.

#ifndef NIRVANA_CORE_WINDOWS_POSTOFFICE_H_
#define NIRVANA_CORE_WINDOWS_POSTOFFICE_H_

#include "MailslotReader.h"
#include "ThreadPostman.h"
#include "ThreadPool.h"

namespace Nirvana {
namespace Core {
namespace Windows {

/// Template class for multithreaded mailslot receiver.
/// \tparam T Derived class.
/// \tparam BUF_SIZE Size of message buffer. Maximal message size.
/// \tparam Thr Thread class. Thread must start in the constructor and join in the destructor.
/// Thread constructor gets reference to PostOfficeBase as parameter./// \tparam PRIORITY Priority of the worker threads.

/// Thread procedure must call PostOffice::thread_proc() method.
template <class T, size_t BUF_SIZE, int PRIORITY>
class PostOffice :
	public MailslotReader,
	public ThreadPool <CompletionPort, ThreadPostman>
{
	static const size_t MAX_WORDS = (BUF_SIZE + sizeof (LONG_PTR) - 1) / sizeof (LONG_PTR);
	static const size_t BUF_SIZE = MAX_WORDS * sizeof (LONG_PTR);
	typedef ThreadPool <CompletionPort, ThreadPostman> Pool;

public:
	/// Derived class T must override this method to receive messages.
	void received (void* message, DWORD size) NIRVANA_NOEXCEPT
	{}

	/// Put the post office to work.
	/// Mailslot must be already created by the create_mailslot() call.
	/// If exception occurs, terminate() will be called internally.
	void start ()
	{
		// Start reading.
		MailslotReader::start (*static_cast <CompletionPort*> (this));
		Pool::start (PRIORITY);
	}

	/// Terminate the post office work.
	void terminate () NIRVANA_NOEXCEPT
	{
		MailslotReader::terminate ();
		Pool::terminate ();
	}

protected:
	PostOffice () :
		MailslotReader (Pool::thread_count (), BUF_SIZE)
	{}

private:
	virtual void received (OVERLAPPED* ovl, DWORD size) NIRVANA_NOEXCEPT
	{
		// Copy message to stack
		LONG_PTR buf [MAX_WORDS];
		LONG_PTR* msg = (LONG_PTR*)data (ovl);
		real_copy (msg, msg + (size + sizeof (LONG_PTR) - 1) / sizeof (LONG_PTR), buf);

		// Enqueue buffer to reading a next message.
		enqueue_buffer (ovl);

		// Process message
		static_cast <T*> (this)->received (buf, size);
	}
};

}
}
}

#endif
