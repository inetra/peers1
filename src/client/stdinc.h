/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if !defined(STDINC_H)
#define STDINC_H

#include "memcpy_amd.h"

#ifndef _DEBUG
# define _SECURE_SCL  0
#endif

// This enables stlport's debug mode (and slows it down to a crawl...)
//#define _STLP_DEBUG 1
//#define _STLP_USE_MALLOC 1
//#define _STLP_USE_NEWALLOC 1
//#define _STLP_LEAKS_PEDANTIC 1

// --- Shouldn't have to change anything under here...

#ifndef _REENTRANT
# define _REENTRANT 1
#endif

#ifndef BZ_NO_STDIO
#define BZ_NO_STDIO 1
#endif

#ifndef USE_SYS_STL
#define USE_SYS_STL 1
#endif

#ifdef _MSC_VER

//disable the deprecated warnings for the CRT functions.
#define _CRT_SECURE_NO_DEPRECATE 1
#define _ATL_SECURE_NO_DEPRECATE 1
#define _CRT_NON_CONFORMING_SWPRINTFS 1

# pragma warning(disable: 4711) // function 'xxx' selected for automatic inline expansion
# pragma warning(disable: 4786) // identifier was truncated to '255' characters in the debug information
# pragma warning(disable: 4290) // C++ Exception Specification ignored
# pragma warning(disable: 4127) // constant expression
# pragma warning(disable: 4710) // function not inlined
# pragma warning(disable: 4503) // decorated name length exceeded, name was truncated
# pragma warning(disable: 4428) // universal-character-name encountered in source
# pragma warning(disable: 4201) // nonstadard extension used : nameless struct/union

typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;

typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

# ifndef CDECL
#  define CDECL _cdecl
# endif

#else // _MSC_VER

# ifndef CDECL
#  define CDECL
# endif

#endif // _MSC_VER

typedef uint32_t tick_t;

#ifdef _WIN32
# define _WIN32_WINNT 0x0501
# define _WIN32_IE	0x0501
# define WINVER 0x501
# define _USE_32BIT_TIME_T

#define STRICT
#define WIN32_LEAN_AND_MEAN

#if _MSC_VER == 1400
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
//disable the deprecated warnings for the crt functions.
#pragma warning(disable: 4996)
#endif

#include <winsock2.h>

#include <windows.h>
#include <mmsystem.h>

#include <tchar.h>
#include <shlobj.h>

#else
#include <unistd.h>
#include <stdint.h>
#endif

#undef memcpy
#undef memset
#undef memzero
#define memcpy memcpy2
#define memset memset2
#define memzero memzero2

#ifdef _MSC_VER
#include <crtdbg.h>
#else
#include <assert.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <memory.h>
#include <sys/types.h>
#include <time.h>
#include <locale.h>

#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <deque>
#include <list>
#include <utility>
#include <functional>
#include <memory>
#include <numeric>
#include <limits>

#ifdef _STLPORT_VERSION
# define HASH_SET hash_set
# define HASH_MAP hash_map
# define HASH_MULTIMAP hash_multimap
# define HASH_SET_X(key, hfunc, eq, order) hash_set<key, hfunc, eq >
# define HASH_MAP_X(key, type, hfunc, eq, order) hash_map<key, type, hfunc, eq >
# define HASH_MULTIMAP_X(key, type, hfunc, eq, order) hash_multimap<key, type, hfunc, eq >

#include <hash_map>
#include <hash_set>
using namespace std;

#elif defined(__GLIBCPP__) || defined(__GLIBCXX__)  // Using GNU C++ library?
# define HASH_SET hash_set
# define HASH_MAP hash_map
# define HASH_MULTIMAP hash_multimap
# define HASH_SET_X(key, hfunc, eq, order) hash_set<key, hfunc, eq >
# define HASH_MAP_X(key, type, hfunc, eq, order) hash_map<key, type, hfunc, eq >
# define HASH_MULTIMAP_X(key, type, hfunc, eq, order) hash_multimap<key, type, hfunc, eq >

#include <ext/hash_map>
#include <ext/hash_set>
#include <ext/functional>
using namespace std;
using namespace __gnu_cxx;

// GNU C++ library doesn't have hash(std::string) or hash(long long int)
namespace __gnu_cxx {
	template<> struct hash<std::string> {
		size_t operator()(const std::string& x) const
			{ return hash<const char*>()(x.c_str()); }
	};
	template<> struct hash<long long int> {
		size_t operator()(long long int x) const { return x; }
	};
}

#elif defined(_MSC_VER)  // Assume the msvc stl
# define HASH_SET hash_set
# define HASH_MAP hash_map
# define HASH_MULTIMAP hash_multimap
# define HASH_SET_X(key, hfunc, eq, order) hash_set<key, hfunc >
# define HASH_MAP_X(key, type, hfunc, eq, order) hash_map<key, type, hfunc >
# define HASH_MULTIMAP_X(key, type, hfunc, eq, order) hash_multimap<key, type, hfunc >

#include <hash_map>
#include <hash_set>
using namespace std;
using namespace stdext;

#else
# define HASH_SET set
# define HASH_MAP map
# define HASH_SET_X(key, hfunc, eq, order)
# define HASH_MAP_X(key, type, hfunc, eq, order) map<key, type, order >
# define HASH_MULTIMAP multimap
# define HASH_MULTIMAP_X(key, type, hfunc, eq, order) multimap<key, type, order >
#endif

#endif // !defined(STDINC_H)

/**
 * @file
 * $Id: stdinc.h,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
