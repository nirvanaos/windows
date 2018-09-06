// Nirvana project
// Windows implementation.
// PostOffice template.
// Used to receive messages from mailslot.

#ifndef NIRVANA_CORE_WINDOWS_POSTOFFICE_H_
#define NIRVANA_CORE_WINDOWS_POSTOFFICE_H_

#include "../core.h"
#include <ORB.h>
#include <memory>

namespace Nirvana {
namespace Core {
namespace Windows {

class PostOfficeBase
{
	PostOfficeBase (const PostOfficeBase&);
	PostOfficeBase& operator = (const PostOfficeBase&);
public:
	class Buffer :
		private OVERLAPPED
	{
	public:
		/// Returns address of buffer contained incoming message.
		const void* message () const
		{
			return &enqueued_ + 1;	// Data buffer follows after this base class.
		}

		/// Returns size of incoming message.
		DWORD size () const
		{
			return size_;
		}

	protected:
		Buffer () :
			enqueued_ (false)
		{
			memset (static_cast <OVERLAPPED*> (this), 0, sizeof (OVERLAPPED));
		}

		void enqueue (HANDLE h, DWORD size)
		{
			if (ReadFile (h, const_cast <void*> (message ()), size, nullptr, this))
				enqueued_ = true;
		}

		void cancel (HANDLE h)
		{
			if (enqueued_) {
				CancelIoEx (h, this);
				enqueued_ = false;
			}
		}

	private:
		friend class PostOfficeBase;
		union
		{
			struct
			{
				bool enqueued_;
				DWORD size_;
			};
			ULONGLONG aligned64_;	// To provide alignment to 64 word.
		};
	};

	/// Threads call this method to get incoming messages.
	/// \param size The size of incoming message.
	/// \returns Buffer* containing incoming message.
	/// If returns nullptr, thread must terminate.
	Buffer* get_message ();

	void enqueue_buffer (Buffer& buffer)
	{
		buffer.enqueue (mailslot_, max_msg_size_);
	}

	void cancel_buffer (Buffer& buffer)
	{
		buffer.cancel (mailslot_);
	}

protected:
	PostOfficeBase (unsigned max_msg_size) :
		running_ (false),
		mailslot_ (INVALID_HANDLE_VALUE),
		completion_port_ (nullptr),
		max_msg_size_ (max_msg_size)
	{}
	
	void initialize (LPCWSTR mailslot_name); // Returns number of threads (processor cores).
	
	void terminate_begin ()
	{
		running_ = false;
	}

	void close_handles ();

	unsigned thread_count () const
	{
		return thread_count_;
	}

protected:
	HANDLE mailslot_;
	HANDLE completion_port_;

private:
	bool running_;
	DWORD max_msg_size_;
	unsigned thread_count_;
};

/// Template class for multithreaded mailslot receiver.
/// \param BUF_SIZE Size of message buffer. Maximal message size.
/// \param Thread Thread class. Thread must start in the constructor and join in the destructor.
/// Thread constructor gets reference to PostOfficeBase as parameter.
/// Thread procedure must call PostOfficeBase::get_message() method to wait for incoming messages.
/// If PostOfficeBase::get_message() returned buffer then thread must get message and enqueue buffer
/// by call PostOfficeBase::enqueue_buffer().
template <unsigned BUF_SIZE, class Thread>
class PostOffice :
	public PostOfficeBase
{
public:
	PostOffice () :
		PostOfficeBase (BUF_SIZE),
		postmasters_ (nullptr)
	{}

	~PostOffice ()
	{
		terminate ();
	}

	void initialize (LPCWSTR mailslot_name)
	{
		try {
			PostOfficeBase::initialize (mailslot_name);

			Allocator allocator;
			postmasters_ = allocator.allocate (thread_count ());
			for (Postmaster* p = postmasters_, *end = p + thread_count (); p != end; ++p) {
				allocator.construct (p, *this);
			}

			for (Postmaster* p = postmasters_, *end = p + thread_count (); p != end; ++p) {
				enqueue_buffer (*p);
			}

		} catch (...) {
			terminate ();
			throw;
		}
	}

	void terminate ();

private:
	class Postmaster :
		public Buffer
	{
	public:
		Postmaster (PostOfficeBase& obj) :
			thread_ (obj)
		{}

	private:
		BYTE buffer_ [BUF_SIZE];
		Thread thread_;
	};

#ifdef UNIT_TEST
	typedef ::std::allocator <Postmaster> Allocator;
#else
	typedef CoreAllocator <Postmaster> Allocator;
#endif

	Postmaster* postmasters_;
};

template <unsigned BUF_SIZE, class Thread>
void PostOffice <BUF_SIZE, Thread>::terminate ()
{
	terminate_begin ();	// Reset running flag.
	for (Postmaster* p = postmasters_, *end = p + thread_count (); p != end; ++p) {
		cancel_buffer (*p);
	}
	close_handles ();	
	// GetQueuedCompletionStatus will return error after closing the completion port.
	
	Allocator allocator;
	for (Postmaster* p = postmasters_, *end = p + thread_count (); p != end; ++p) {
		allocator.destroy (p); // Thread joins in destructor.
	}
	allocator.deallocate (postmasters_, thread_count ());
	postmasters_ = nullptr;
}

}
}
}

#endif
