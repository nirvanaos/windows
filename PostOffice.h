// Nirvana project
// Windows implementation.
// PostOffice template.
// Used to receive messages from mailslot.

#ifndef NIRVANA_CORE_WINDOWS_POSTOFFICE_H_
#define NIRVANA_CORE_WINDOWS_POSTOFFICE_H_

#include "MailslotReader.h"
#include <real_copy.h>

namespace Nirvana {
namespace Core {
namespace Windows {

/// Template class for multithreaded mailslot receiver.
/// \tparam T Derived class.
/// \tparam BUF_SIZE Size of message buffer. Maximal message size.
/// \tparam Thread Thread class. Thread must start in the constructor and join in the destructor.
/// Thread constructor gets reference to PostOfficeBase as parameter.
/// Thread procedure must call PostOffice::dispatch() method while it returns true.
template <class T, unsigned BUF_SIZE, class Thread>
class PostOffice :
	public ThreadPool <Thread>,
	public MailslotReader
{
	static const size_t MAX_WORDS = (BUF_SIZE + sizeof (LONG_PTR) - 1) / sizeof (LONG_PTR);
public:
	/// Derived class must override this method to receive messages.
	void received (void* message, DWORD size)
	{}

	/// Destructor calls terminate().
	~PostOffice ()
	{
		terminate ();
	}

	/// Put the post office to work.
	/// \param mailslot_name Name of the mailslot for incoming messages.
	/// \returns `true` on success. `false` if mailslot already exists.
	bool start (LPCWSTR mailslot_name)
	{
		// Start reading. If exception occurs, terminate will be called internally, so we move it out of try-catch.
		if (!MailslotReader::start (mailslot_name, MAX_WORDS * sizeof (LONG_PTR), *static_cast <CompletionPort*> (this)))
			return false;
		try {
			// Start threads
			ThreadPool <Thread>::start ();
		} catch (...) {
			terminate ();
			throw;
		}
		return true;
	}

	/// Put the post office to work.
	/// \param prefix Mailslot name prefix.
	/// \id Unique id of the mailslot. Usually it is id of the current process.
	template <size_t PREFIX_SIZE>
	void start (const WCHAR (&prefix) [PREFIX_SIZE], DWORD id)
	{
		// Start reading. If exception occurs, terminate will be called internally, so we move it out of try-catch.
		MailslotReader::start (prefix, id, MAX_WORDS * sizeof (LONG_PTR), *static_cast <CompletionPort*> (this));
		try {
			// Start threads
			ThreadPool <Thread>::start ();
		} catch (...) {
			terminate ();
			throw;
		}
	}

	/// Terminate the post office work.
	virtual void terminate ()
	{
		ThreadPool <Thread>::terminate ();
		MailslotReader::terminate ();
	}

private:
	virtual void received (OVERLAPPED* ovl, DWORD size)
	{
		LONG_PTR buf [MAX_WORDS];
		LONG_PTR* msg = (LONG_PTR*)data (ovl);
		real_copy (msg, msg + (size + sizeof (LONG_PTR) - 1) / sizeof (LONG_PTR), buf);
		enqueue_buffer (ovl);
		static_cast <T*> (this)->received (buf, size);
	}
};

}
}
}

#endif
