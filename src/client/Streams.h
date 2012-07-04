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

#if !defined(STREAMS_H)
#define STREAMS_H

#include "SettingsManager.h"
//[-]PPA [Doxygen 1.5.1]  #include "Exception.h"
STANDARD_EXCEPTION(FileException);

/**
 * A naive output stream. We don't use the stl ones to avoid compiling STLPort,
 * besides this is a lot more lightweight (and less flexible)...
 */
class OutputStream {
public:
	OutputStream() { }
	virtual ~OutputStream() throw() { }
	
	/**
	 * @return The actual number of bytes written. len bytes will always be
	 *         consumed, but fewer or more bytes may actually be written,
	 *         for example if the stream is being compressed.
	 */
	virtual size_t write(const void* buf, size_t len) throw(Exception) = 0;
	/**
	 * This must be called before destroying the object to make sure all data
	 * is properly written (we don't want destructors that throw exceptions
	 * and the last flush might actually throw). Note that some implementations
	 * might not need it...
	 */
	virtual size_t flush() throw(Exception) = 0;

	size_t write(const string& str) throw(Exception) { return write(str.c_str(), str.size()); }
private:
	OutputStream(const OutputStream&);
	OutputStream& operator=(const OutputStream&);
};

class InputStream {
public:
	InputStream() { }
	virtual ~InputStream() throw() { }
	/**
	 * Call this function until it returns 0 to get all bytes.
	 * @return The number of bytes read. len reflects the number of bytes
	 *		   actually read from the stream source in this call.
	 */
	virtual size_t read(void* buf, size_t& len) throw(Exception) = 0;
private:
	InputStream(const InputStream&);
	InputStream& operator=(const InputStream&);
};

class MemoryInputStream : public InputStream {
public:
	MemoryInputStream(const uint8_t* src, size_t len) : pos(0), size(len), buf(new uint8_t[len]) {
		memcpy(buf, src, len);
	}
	MemoryInputStream(const string& src) : pos(0), size(src.size()), buf(new uint8_t[src.size()]) {
		memcpy(buf, src.data(), src.size());
	}

	~MemoryInputStream() throw() {
		delete[] buf;
	}

	size_t read(void* tgt, size_t& len) throw(Exception) {
		len = min(len, size - pos);
		memcpy(tgt, buf+pos, len);
		pos += len;
		return len;
	}

	size_t getSize() const { return size; }

private:
	size_t pos;
	size_t size;
	uint8_t* buf;
};

class IOStream : public InputStream, public OutputStream {
#ifdef __INTEL_COMPILER
public:
	virtual ~IOStream() throw(){ }
#endif
};

template<bool managed>
class LimitedInputStream : public InputStream {
public:
	LimitedInputStream(InputStream* is, int64_t aMaxBytes) : s(is), maxBytes(aMaxBytes) {
	}
	~LimitedInputStream() throw() { if(managed) delete s; }

	size_t read(void* buf, size_t& len) throw(FileException) {
		dcassert(maxBytes >= 0);
		len = (size_t)min(maxBytes, (int64_t)len);
		if(len == 0)
			return 0;
		size_t x = s->read(buf, len);
		maxBytes -= x;
		return x;
	}

private:
	InputStream* s;
	int64_t maxBytes;
};

template<bool managed>
class BufferedOutputStream : public OutputStream {
public:
	using OutputStream::write;

	BufferedOutputStream(OutputStream* aStream, size_t aBufSize = SETTING(BUFFER_SIZE) * 1024) : s(aStream), pos(0), bufSize(aBufSize), buf(aBufSize) { }
	~BufferedOutputStream() throw() {
		try {
			// We must do this in order not to lose bytes when a download
			// is disconnected prematurely
			flush();
		} catch(const Exception&) {
		}
		if(managed) delete s;
	}

	size_t flush() throw(Exception) {
		if(pos > 0)
			s->write(buf, pos);
		pos = 0;
		s->flush();
		return 0;
	}

	size_t write(const void* wbuf, size_t len) throw(Exception) {
		uint8_t* b = (uint8_t*)wbuf;
		size_t l2 = len;
		while(len > 0) {
			if(pos == 0 && len >= bufSize) {
				s->write(b, len);
				break;
			} else {
				size_t n = min(bufSize - pos, len);
				memcpy(buf + pos, b, n);
				b += n;
				pos += n;
				len -= n;
				if(pos == bufSize) {
					s->write(buf, bufSize);
					pos = 0;
				}
			}
		}
		return l2;
	}
private:
	OutputStream* s;
	size_t pos;
	size_t bufSize;
	AutoArray<uint8_t> buf;
};

class StringOutputStream : public OutputStream {
public:
	StringOutputStream(string& out) : str(out) { }
	~StringOutputStream() throw() { }
	using OutputStream::write;

	size_t flush() throw(Exception) { return 0; }
	size_t write(const void* buf, size_t len) throw(Exception) {
		str.append((char*)buf, len);
		return len;
	}
private:
	string& str;
};


#endif // !defined(STREAMS_H)

/**
* @file
* $Id: Streams.h,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
*/
