// Nirvana project
// Windows implementation.
// PostOffice template.
// Used to receive messages from mailslot.

#ifndef NIRVANA_CORE_WINDOWS_POSTOFFICE_H_
#define NIRVANA_CORE_WINDOWS_POSTOFFICE_H_

#include "../core.h"
#include <memory>

namespace Nirvana {
namespace Core {
namespace Windows {

/// Base class for PostOffice template.
/// This class is used by post office threads as an interface to PostOffice object.
class PostOfficeBase
{
public:
	/// Buffer with one incoming message.
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

	private:
		friend class PostOfficeBase;

		void enqueue (HANDLE h, DWORD size);

		void cancel (HANDLE h)
		{
			if (enqueued_) {
				CancelIoEx (h, this);
				enqueued_ = false;
			}
		}

	private:
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
	/// \returns Buffer* containing incoming message. If returns nullptr, thread must terminate.
	/// \throws ::CORBA::INTERNAL
	Buffer* get_message ();

	/// After reading the message from buffer, before all further processing,
	/// the buffer have to be returned to pool by enqueue_buffer call.
	/// \param[in] buffer The buffer acquired by get_message call.
	/// \throws ::CORBA::INTERNAL
	void enqueue_buffer (Buffer& buffer);

	/// Returns number of working threads.
	unsigned thread_count () const
	{
		return thread_count_;
	}

protected:
	PostOfficeBase (unsigned max_msg_size) :
		running_ (false),
		mailslot_ (INVALID_HANDLE_VALUE),
		completion_port_ (nullptr),
		max_msg_size_ (max_msg_size)
	{}
	
	bool initialize (LPCWSTR mailslot_name);
	
	void terminate_begin ()
	{
		running_ = false;
	}

	void cancel_buffer (Buffer& buffer)
	{
		buffer.cancel (mailslot_);
	}

	void close_handles ();

private:
	/// Deprecated
	PostOfficeBase (const PostOfficeBase&);
	/// Deprecated
	PostOfficeBase& operator = (const PostOfficeBase&);

private:
	HANDLE mailslot_;
	HANDLE completion_port_;
	DWORD max_msg_size_;
	unsigned thread_count_;
	bool running_;
};

/// Template class for multithreaded mailslot receiver.
/// \tparam BUF_SIZE Size of message buffer. Maximal message size.
/// \tparam Thread Thread class. Thread must start in the constructor and join in the destructor.
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

	/// Destructo calls terminate().
	~PostOffice ()
	{
		terminate ();
	}

	/// Put the post office to work.
	/// \param mailslot_name Name of the mailslot for incoming messages.
	/// \returns `true` on success. `false` if mailslot already exists.
	bool initialize (LPCWSTR mailslot_name)
	{
		try {
			if (!PostOfficeBase::initialize (mailslot_name))
				return false;

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
		return true;
	}

	/// Terminate the post office work.
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

	// Cancel enqueued buffers.
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
