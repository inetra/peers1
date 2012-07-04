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
#pragma once

#define DCVERSIONSTRING "0.699"

//[+] Drakon
#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4512)
#pragma warning (disable : 4505)
#endif
//End
#ifdef __INTEL_COMPILER
# pragma warning(disable: 383) //value copied to temporary, reference to temporary used
# pragma warning(disable: 981) //operands are evaluated in unspecified order
# pragma warning(disable: 1418) //external definition with no prior declaration
# pragma warning(disable: 69) //integer conversion resulted in truncation
# pragma warning(disable: 810)
# pragma warning(disable: 811)
# pragma warning(disable: 504)
# pragma warning(disable: 809)
# pragma warning(disable: 654)
# pragma warning(disable: 181)
# pragma warning(disable: 304)
# pragma warning(disable: 444)
# pragma warning(disable: 373)
# pragma warning(disable: 174)
# pragma warning(disable: 1599)
# pragma warning(disable: 1461)
# pragma warning(disable: 869)
# pragma warning(disable: 584) //warning #584: omission of exception specification is incompatible with previous function 
#endif

#if !defined(DC_PLUS_PLUS_H)
#define DC_PLUS_PLUS_H

//[+]PPA
// #define PPA_INCLUDE_SSL 
// #define PPA_INCLUDE_DEAD_CODE
// #define PPA_INCLUDE_PG 
// #define PPA_INCLUDE_BITZI_LOOKUP 
// #define PPA_INCLUDE_DNS
// #define PPA_INCLUDE_COUNTRYLIST
#define PPA_INCLUDE_DROP_SLOW
#define PPA_INCLUDE_IPFILTER
//#define PPA_INCLUDE_NETLIMITER

#define COUNTOF(a)  (sizeof(a)/sizeof((a)[0])) //[+]PPA _countof helper

template <class T> inline void SafeRelease(T* & p) 
{ 
 if(p) 
  { 
	  p->Release(); 
	  p = NULL; 
  }
}
template <class T> inline void DestroyAndDetachWindow(T & p) 
{ 
 if(p) 
  { 
  p.DestroyWindow(); 
  p.Detach();
 }
}

#ifdef _WIN32
# define snprintf _snprintf
# define snwprintf _snwprintf
#endif

#ifdef _DEBUG
extern void CDECL debugTrace(const char* format, ...);
#define dcdebug debugTrace
#ifdef _MSC_VER
#define dcassert(exp) \
do { if (!(exp)) { \
	dcdebug("Assertion hit in %s(%d): " #exp "\n", __FILE__, __LINE__); \
	if(1 == _CrtDbgReport(_CRT_ASSERT, __FILE__, __LINE__, NULL, #exp)) \
_CrtDbgBreak(); } } while(false)
#define dcasserta(exp) dcassert(0)
#else
#define dcasserta(exp) assert(exp)
#define dcassert(exp) assert(exp)
#endif
#define dcdrun(exp) exp
#else //_DEBUG
#ifdef _MSC_VER
#define dcasserta(exp) __assume(exp)
#else
#define dcasserta(exp)
#endif // _WIN32
#define dcdebug if (false) printf
#define dcassert(exp)
#define dcdrun(exp)
#endif //_DEBUG
#define __dcdebug if (false) printf

// Make sure we're using the templates from algorithm...
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

typedef vector<string> StringList;
typedef StringList::iterator StringIter;
typedef StringList::const_iterator StringIterC;

typedef pair<string, string> StringPair;
typedef vector<StringPair> StringPairList;
typedef StringPairList::iterator StringPairIter;

typedef HASH_MAP<string, string> StringMap;
typedef StringMap::iterator StringMapIter;
typedef StringMap::const_iterator StringMapIterC;

//[-]PPA typedef HASH_SET<string> StringSet;
//[-]PPA typedef StringSet::iterator StringSetIter;

typedef vector<wstring> WStringList;
typedef WStringList::iterator WStringIter;
typedef WStringList::const_iterator WStringIterC;

typedef pair<wstring, wstring> WStringPair;
typedef vector<WStringPair> WStringPairList;
typedef WStringPairList::iterator WStringPairIter;

typedef vector<uint8_t> ByteVector;

#if defined(_MSC_VER) || defined(__MINGW32__)
#define _LL(x) x##ll
#define _ULL(x) x##ull
#define I64_FMT "%I64d"
#define U64_FMT "%I64d"
#define X64_FMT "%08I64x"

#elif defined(SIZEOF_LONG) && SIZEOF_LONG == 8
#define _LL(x) x##l
#define _ULL(x) x##ul
#define I64_FMT "%ld"
#define U64_FMT "%ld"
#else
#define _LL(x) x##ll
#define _ULL(x) x##ull
#define I64_FMT "%lld"
#define U64_FMT "%lld"
#endif

#ifdef _WIN32

# define PATH_SEPARATOR '\\'
# define PATH_SEPARATOR_STR "\\"

#else

# define PATH_SEPARATOR '/'
# define PATH_SEPARATOR_STR "/"

#endif


typedef HASH_MAP<wstring, wstring> WStringMap;
typedef WStringMap::iterator WStringMapIter;

#ifdef UNICODE

typedef wstring tstring;
typedef WStringList TStringList;
typedef WStringIter TStringIter;
typedef WStringIterC TStringIterC;

typedef WStringPair TStringPair;
typedef WStringPairIter TStringPairIter;
typedef WStringPairList TStringPairList;

typedef WStringMap TStringMap;
typedef WStringMapIter TStringMapIter;

#else

typedef string tstring;
typedef StringList TStringList;
typedef StringIter TStringIter;
typedef StringIterC TStringIterC;

typedef StringPair TStringPair;
typedef StringPairIter TStringPairIter;
typedef StringPairList TStringPairList;

typedef StringMap TStringMap;
typedef StringMapIter TStringMapIter;
typedef StringMapIterC TStringMapIterC;
#endif

extern bool g_RunningUnderWine;

#define __DEPRECATED __declspec(deprecated)

#endif // !defined(DC_PLUS_PLUS_H)

/**
 * @file
 * $Id: DCPlusPlus.h,v 1.8.2.1 2008/12/21 14:29:40 alexey Exp $
 */
