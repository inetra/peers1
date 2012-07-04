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

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "Text.h"
#include "Util.h"

#ifndef _WIN32
#include <errno.h>
#include <iconv.h>

#ifndef ICONV_CONST
 #define ICONV_CONST
#endif

#endif

string Text::systemCharset;
const string Text::utf8 = "UTF-8"; // optimization

void Text::initialize() {
setlocale(LC_ALL, "");

	char *ctype = setlocale(LC_CTYPE, NULL);
	if(ctype) {
		systemCharset = string(ctype);
	} else {
		dcdebug("Unable to determine the program's locale");
	}
}

int Text::utf8ToWc(const char* str, wchar_t& c) {
	uint8_t c0 = (uint8_t)str[0];
	if(c0 & 0x80) {									// 1xxx xxxx
		if(c0 & 0x40) {								// 11xx xxxx
			if(c0 & 0x20) {							// 111x xxxx
				if(c0 & 0x10) {						// 1111 xxxx
					int n = -4;
					if(c0 & 0x08) {					// 1111 1xxx
						n = -5;
						if(c0 & 0x04) {				// 1111 11xx
							if(c0 & 0x02) {			// 1111 111x
								return -1;
							}
							n = -6;
						}
					}
					int i = -1;
					while(i > n && (str[abs(i)] & 0x80) == 0x80)
						--i;
					return i;
				} else {		// 1110xxxx
					uint8_t c1 = (uint8_t)str[1];
					if((c1 & (0x80 | 0x40)) != 0x80)
						return -1;

					uint8_t c2 = (uint8_t)str[2];
					if((c2 & (0x80 | 0x40)) != 0x80)
						return -2;

					// Ugly utf-16 surrogate catch
					if((c0 & 0x0f) == 0x0d && (c1 & 0x3c) >= (0x08 << 2))
						return -3;

					// Overlong encoding
					if(c0 == (0x80 | 0x40 | 0x20) && (c1 & (0x80 | 0x40 | 0x20)) == 0x80)
						return -3;

					c = (((wchar_t)c0 & 0x0f) << 12) |
						(((wchar_t)c1 & 0x3f) << 6) |
						((wchar_t)c2 & 0x3f);

					return 3;
				}
			} else {				// 110xxxxx
				uint8_t c1 = (uint8_t)str[1];
				if((c1 & (0x80 | 0x40)) != 0x80)
					return -1;

				// Overlong encoding
				if((c0 & ~1) == (0x80 | 0x40))
					return -2;

				c = (((wchar_t)c0 & 0x1f) << 6) |
					((wchar_t)c1 & 0x3f);
				return 2;
			}
		} else {					// 10xxxxxx
			return -1;
		}
	} else {						// 0xxxxxxx
		c = (unsigned char)str[0];
		return 1;
	}
}

void Text::wcToUtf8(wchar_t c, string& str) {
	if(c >= 0x0800) {
		str += (char)(0x80 | 0x40 | 0x20  | (c >> 12));
		str += (char)(0x80 | ((c >> 6) & 0x3f));
		str += (char)(0x80 | (c & 0x3f));
	} else if(c >= 0x0080) {
		str += (char)(0x80 | 0x40 | (c >> 6));
		str += (char)(0x80 | (c & 0x3f));
	} else {
		str += (char)c;
	}
}

string& Text::acpToUtf8(const string& str, string& tmp) throw() {
	wstring wtmp;
	return wideToUtf8(acpToWide(str, wtmp), tmp);
}

wstring& Text::acpToWide(const string& str, wstring& tmp) throw() {
	if(str.empty())
		return tmp;
#ifdef _WIN32
	int n = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), (int)str.length(), NULL, 0);
	if(n == 0) {
		return tmp;
	}

	tmp.resize(n);
	n = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str.c_str(), (int)str.length(), &tmp[0], n);
	if(n == 0) {
		tmp.clear();
		return tmp;
	}
	return tmp;
#else
	size_t rv;
	wchar_t wc;
	const char *src = str.c_str();
	size_t n = str.length() + 1;
	tmp.reserve(n);

	while(n > 0) {
		rv = mbrtowc(&wc, src, n, NULL);
		if(rv == 0 || rv == (size_t)-2) {
			break;
		} else if(rv == (size_t)-1) {
			tmp.push_back(L'_');
			++src;
			--n;
		} else {
			tmp.push_back(wc);
			src += rv;
			n -= rv;
		}
	}
	return tmp;
#endif
}

string& Text::wideToUtf8(const wstring& str, string& tgt) throw() {
	if(str.empty())
		return tgt;
#ifdef _WIN32
	int size = 0;
	tgt.resize( str.length() * 2 );

	while( ( size = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.length(), &tgt[0], tgt.length(), NULL, NULL) ) == 0 ){
		if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
			tgt.resize( tgt.size() * 2 );
		else
			break;
	}
	
	tgt.resize( size );
	return tgt;
#else
	string::size_type n = str.length();
	for(string::size_type i = 0; i < n; ++i) {
		wcToUtf8(str[i], tgt);
	}
	return tgt;
#endif
}

string& Text::wideToAcp(const wstring& str, string& tmp) throw() {
	if(str.empty())
		return tmp;
#ifdef _WIN32
	int n = WideCharToMultiByte(CP_ACP, 0, str.c_str(), (int)str.length(), NULL, 0, NULL, NULL);
	if(n == 0) {
		return tmp;
	}

	tmp.resize(n);
	n = WideCharToMultiByte(CP_ACP, 0, str.c_str(), (int)str.length(), &tmp[0], n, NULL, NULL);
	if(n == 0) {
		tmp.clear();
		return tmp;
	}
	return tmp;
#else
	const wchar_t* src = str.c_str();
	int n = wcsrtombs(NULL, &src, 0, NULL);
	if(n < 1) {
		return tmp;
	}
	src = str.c_str();
	tmp.resize(n);
	n = wcsrtombs(&tmp[0], &src, n, NULL);
	if(n < 1) {
		tmp.clear();
		return tmp;
	}
	return tmp;
#endif
}

bool Text::validateUtf8(const string& str) throw() {
	string::size_type i = 0;
	while(i < str.length()) {
		wchar_t dummy = 0;
		int j = utf8ToWc(&str[i], dummy);
		if(j < 0)
			return false;
		i += j;
	}
	return true;
}

string& Text::utf8ToAcp(const string& str, string& tmp) throw() {
	wstring wtmp;
	return wideToAcp(utf8ToWide(str, wtmp), tmp);
}

wstring& Text::utf8ToWide(const string& str, wstring& tgt) throw() {
#ifdef _WIN32
	int size = 0;
	tgt.resize( str.length()+1 );
	while( ( size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &tgt[0], (int)tgt.length()) ) == 0 ){
		if( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) {
			tgt.resize( tgt.size()*2 );
		} else {
			break;
		}

	}
	tgt.resize( size );
	return tgt;
#else
	tgt.reserve(str.length());
	string::size_type n = str.length();
	for(string::size_type i = 0; i < n; ) {
		wchar_t c = 0;
		int x = utf8ToWc(str.c_str() + i, c);
		if(x < 0) {
			tgt += '_';
			i += abs(x);
		} else {
			i += x;
			tgt += c;
		}
	}
	return tgt;
#endif	
}

wchar_t Text::toLower(wchar_t c) throw() {
#ifdef _WIN32
		return (wchar_t)CharLowerW((LPWSTR)c);
#else
		return (wchar_t)towlower(c);
#endif
}

wstring& Text::toLower(const wstring& str, wstring& tmp) throw() {
	if(str.empty()) return tmp;
	tmp.reserve(str.length());
	wstring::const_iterator end = str.end();
	for(wstring::const_iterator i = str.begin(); i != end; ++i) {
		tmp += toLower(*i);
	}
	return tmp;
}

string& Text::toLower(const string& str, string& tmp) throw() {
	if(str.empty())
		return tmp;
	tmp.reserve(str.length());
	const char* end = &str[0] + str.length();
	for(const char* p = &str[0]; p < end;) {
		wchar_t c = 0;
		int n = utf8ToWc(p, c);
		if(n < 0) {
			tmp += '_';
			p += abs(n);
		} else {
			p += n;
			wcToUtf8(toLower(c), tmp);
		}
	}
	return tmp;
}

const string& Text::convert(const string& str, string& tmp, const string& fromCharset, const string& toCharset) throw() {
	if(str.empty() || Util::stricmp(fromCharset, toCharset) == 0)
		return str;
	if((fromCharset.empty() || fromCharset == systemCharset) && (toCharset == utf8 || toLower(toCharset) == "utf-8"))
		return acpToUtf8(str, tmp);
	if((toCharset.empty() || toCharset == systemCharset) && (fromCharset == utf8 || toLower(fromCharset) == "utf-8"))
		return utf8ToAcp(str, tmp);
	
#ifdef _WIN32
	// We don't know how to convert arbitrary charsets
	dcdebug("Unknown conversion from %s to %s\n", fromCharset.c_str(), toCharset.c_str());
	return str;
#else
 
	// Initialize the converter
	iconv_t cd = iconv_open(toCharset.c_str(), fromCharset.c_str());
	if(cd == (iconv_t)-1)
		return tmp = str;

	size_t rv;
	size_t len = str.length() * 2; // optimization
	size_t inleft = str.length();
	size_t outleft = len;
	tmp.resize(len);
	const char *inbuf = str.data();
	char *outbuf = (char *)tmp.data();

	while(inleft > 0) {
		rv = iconv(cd, (ICONV_CONST char **)&inbuf, &inleft, &outbuf, &outleft);
		if(rv == (size_t)-1) {
			size_t used = outbuf - tmp.data();
			if(errno == E2BIG) {
				len *= 2;
				tmp.resize(len);
				outbuf = (char *)tmp.data() + used;
				outleft = len - used;
			} else if(errno == EILSEQ) {
				++inbuf;
				--inleft;
				tmp[used] = '_';
			} else {
				tmp.replace(used, inleft, string(inleft, '_'));
				inleft = 0;
			}
		}
	}
	iconv_close(cd);
	if(outleft > 0) {
		tmp.resize(len - outleft);
	}
	return tmp;
#endif
 }
 
/**
 * @file
 * $Id: Text.cpp,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
