#ifndef NIRVANA_CORE_WINDOWS_CMDLINEPARSER_H_
#define NIRVANA_CORE_WINDOWS_CMDLINEPARSER_H_

#include <Heap.h>
#include "win32.h"
#include <shellapi.h>

namespace Nirvana {
namespace Core {
namespace Windows {

class CmdLineParser
{
public:
	CmdLineParser () :
		argv_ (nullptr),
		cb_ (0),
		argc_ (0)
	{
		LPWSTR* argv = CommandLineToArgvW (GetCommandLineW (), &argc_);
		try {
			size_t ccnt = 0;
			for (LPWSTR* arg = argv, *end = argv + argc_; arg != end; ++arg)
				ccnt += to_utf8 (*arg);
			cb_ = argc_ * sizeof (char*) + ccnt;
			char** uarg = argv_ = (char**)g_core_heap->allocate (nullptr, cb_, 0);
			char* buf = (char*)(uarg + argc_);
			for (LPWSTR* arg = argv, *end = argv + argc_; arg != end; ++arg, ++uarg) {
				*uarg = buf;
				size_t cb = to_utf8 (*arg, buf, ccnt);
				buf += cb;
				ccnt -= cb;
			}
		} catch (...) {
			LocalFree (argv);
			throw;
		}
		LocalFree (argv);
	}

	~CmdLineParser ()
	{
		g_core_heap->release (argv_, cb_);
	}

	char** argv ()
	{
		return argv_;
	}

	int argc () const
	{
		return argc_;
	}

private:
	static size_t to_utf8 (LPCWSTR ws, LPSTR us = nullptr, size_t cb = 0)
	{
		return WideCharToMultiByte (CP_UTF8, WC_ERR_INVALID_CHARS | WC_NO_BEST_FIT_CHARS, ws, -1, us, (int)cb, nullptr, nullptr);
	}

private:
	char** argv_;
	size_t cb_;
	int argc_;
};

}
}
}

#endif
