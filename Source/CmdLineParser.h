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
#ifndef NIRVANA_CORE_WINDOWS_CMDLINEPARSER_H_
#define NIRVANA_CORE_WINDOWS_CMDLINEPARSER_H_
#pragma once

//#include <Heap.h>
#include <memory>
#include "win32.h"
#include <shellapi.h>

namespace Nirvana {
namespace Core {
namespace Windows {

template <template <class C> class Allocator = std::allocator>
class CmdLineParser
{
public:
	CmdLineParser () :
		argv_ (nullptr),
		cb_ (0),
		argc_ (0)
	{
		LPWSTR* argv = CommandLineToArgvW (GetCommandLineW (), &argc_);
		size_t ccnt = 0;
		for (LPWSTR* arg = argv, *end = argv + argc_; arg != end; ++arg) {
			ccnt += to_utf8 (*arg);
		}
		LPWCH env = GetEnvironmentStringsW ();
		int env_cnt = 0;
		for (LPWCH p = env; *p; p += wcslen (p) + 1) {
			++env_cnt;
			ccnt += to_utf8 (p);
		}
		int ptr_cnt = argc_ + env_cnt + 1;
		cb_ = ptr_cnt * sizeof (char*) + ccnt;
		try {
			char** uarg = argv_ = (char**)Allocator <char>::allocate (cb_, 0);
			char* buf = (char*)(uarg + ptr_cnt);
			for (LPWSTR* arg = argv, *end = argv + argc_; arg != end; ++arg, ++uarg) {
				*uarg = buf;
				size_t cb = to_utf8 (*arg, buf, ccnt);
				buf += cb;
				ccnt -= cb;
			}
			for (LPWCH p = env; *p; p += wcslen (p), ++uarg) {
				*uarg = buf;
				size_t cb = to_utf8 (p, buf, ccnt);
				buf += cb;
				ccnt -= cb;
			}
			*uarg = nullptr;
		} catch (...) {
			LocalFree (argv);
			FreeEnvironmentStringsW (env);
			throw;
		}
		LocalFree (argv);
		FreeEnvironmentStringsW (env);
	}

	~CmdLineParser ()
	{
		Allocator <char>::deallocate ((char*)argv_, cb_);
	}

	char** argv () const
	{
		return argv_;
	}

	int argc () const
	{
		return argc_;
	}

	char** envp () const
	{
		return argc_ + argv_;
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
