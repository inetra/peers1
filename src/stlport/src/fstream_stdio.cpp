/*
 * Copyright (c) 1999
 * Silicon Graphics Computer Systems, Inc.
 *
 * Copyright (c) 1999
 * Boris Fomitchev
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#if defined  (__SUNPPRO_CC)  && !defined (_STLP_NO_NEW_C_HEADERS)
#  include <time.h>
// For sunpro, it chokes if time.h is included through stat.h
#endif

#include <fstream>

#ifdef __CYGWIN__
#  define __int64 long long
#endif

#include <cstdio>
#if !defined(__ISCPP__)
extern "C" {
#  include <sys/stat.h>
}
#endif

#if defined( __MSL__ )
#  include <unix.h>
#endif

#if defined(__ISCPP__)
#  include <c_locale_is/filestat.h>
#endif

#if defined(__BEOS__) && defined(__INTEL__)
#  include <fcntl.h>
#  include <sys/stat.h>         // For _fstat
#endif

#if defined (_STLP_MSVC) || defined (__MINGW32__)
#  include <fcntl.h>
#  define S_IREAD _S_IREAD
#  define S_IWRITE _S_IWRITE
#  define S_IFREG _S_IFREG
     // map permission masks
#  ifndef S_IRUSR
#    define S_IRUSR _S_IREAD
#    define S_IWUSR _S_IWRITE
#  endif
#  ifndef S_IRGRP
#    define S_IRGRP _S_IREAD
#    define S_IWGRP _S_IWRITE
#  endif
#  ifndef S_IROTH
#    define S_IROTH _S_IREAD
#    define S_IWOTH _S_IWRITE
#  endif

#  ifndef O_RDONLY
#    define O_RDONLY _O_RDONLY
#    define O_WRONLY _O_WRONLY
#    define O_RDWR   _O_RDWR
#    define O_APPEND _O_APPEND
#    define O_CREAT  _O_CREAT
#    define O_TRUNC  _O_TRUNC
#    define O_TEXT   _O_TEXT
#    define O_BINARY _O_BINARY
#  endif

#  ifndef O_ACCMODE
#    define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)
#  endif
#endif

const _STLP_fd INVALID_STLP_FD = -1;


#  ifdef __MSL__
#    define _O_TEXT 0x0
#    if !defined( O_TEXT )
#      define O_TEXT _O_TEXT
#    endif
#    define _S_IFREG S_IFREG
#    define S_IREAD        S_IRUSR
#    define S_IWRITE       S_IWUSR
#    define S_IEXEC        S_IXUSR
#    define _S_IWRITE S_IWRITE
#    define _S_IREAD S_IREAD
#    define _open open
#    define _close close
#    define _read read
#    define _write write
#  endif

#include "fstream_impl.h"

#ifdef _STLP_LONG_LONG
#  define ULL(x) ((unsigned _STLP_LONG_LONG)x)
#elif defined(__ISCPP__)
#  include "uint64.h"
#else
#  error "there should be some long long type on the system!"
#endif

_STLP_BEGIN_NAMESPACE

// Compare with streamoff definition in stl/char_traits.h!

#ifdef _STLP_USE_DEFAULT_FILE_OFFSET
#  define FOPEN fopen
#  define FSEEK fseek
#  define FSTAT fstat
#  define STAT  stat
#  define FTELL ftell
#elif defined(_LARGEFILE_SOURCE) || defined(_LARGEFILE64_SOURCE) /* || defined(__USE_FILE_OFFSET64) */ \
     /* || (defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)) */ /* || defined(__sgi) */
#  define FOPEN fopen64
#  define FSEEK fseeko64
#  define FSTAT fstat64
#  define STAT  stat64
#  define FTELL ftello64
#else
#  define OPEN  open
#  define FSEEK fseek
#  define FSTAT fstat
#  define STAT  stat
#  define FTELL ftell
#endif


_STLP_MOVE_TO_PRIV_NAMESPACE

// Helper functions for _Filebuf_base.

bool __is_regular_file(_STLP_fd fd) {
  struct STAT buf;
  return FSTAT(fd, &buf) == 0 && (buf.st_mode & S_IFREG) != 0 ;
}

// Number of characters in the file.
streamoff __file_size(_STLP_fd fd) {
  streamoff ret = 0;

  struct STAT buf;
  if (FSTAT(fd, &buf) == 0 && (buf.st_mode & S_IFREG) != 0)
    ret = buf.st_size > 0 ? buf.st_size : 0;

  return ret;
}

_STLP_MOVE_TO_STD_NAMESPACE

// All version of Unix have mmap and lseek system calls.  Some also have
// longer versions of those system calls to accommodate 64-bit offsets.
// If we're on a Unix system, define some macros to encapsulate those
// differences.

size_t _Filebuf_base::_M_page_size = 4096;

_Filebuf_base::_Filebuf_base()
  : _M_file_id(INVALID_STLP_FD),
    _M_openmode(0),
    _M_is_open(false),
    _M_should_close(false)
{}

void _Filebuf_base::_S_initialize()
{

}

// Return the size of the file.  This is a wrapper for stat.
// Returns zero if the size cannot be determined or is ill-defined.
streamoff _Filebuf_base::_M_file_size()
{
  return _STLP_PRIV __file_size(_M_file_id);
}

bool _Filebuf_base::_M_open(const char* name, ios_base::openmode openmode,
                            long permission)
{
  _STLP_fd file_no;

  if (_M_is_open)
    return false;

  // use FILE-based i/o
  const char* flags;

  switch (openmode & (~ios_base::ate)) {
    case ios_base::out:
    case ios_base::out | ios_base::trunc:
      flags = "w";
      break;

    case ios_base::out | ios_base::binary:
    case ios_base::out | ios_base::trunc | ios_base::binary:
      flags = "wb";
      break;

    case ios_base::out | ios_base::app:
      flags = "a";
      break;

    case ios_base::out | ios_base::app | ios_base::binary:
      flags = "ab";
      break;

    case ios_base::in:
      flags = "r";
      break;

    case ios_base::in | ios_base::binary:
      flags = "rb";
      break;

    case ios_base::in | ios_base::out:
      flags = "r+";
      break;

    case ios_base::in | ios_base::out | ios_base::binary:
      flags = "r+b";
      break;

    case ios_base::in | ios_base::out | ios_base::trunc:
      flags = "w+";
      break;

    case ios_base::in | ios_base::out | ios_base::trunc | ios_base::binary:
      flags = "w+b";
      break;

    default:                      // The above are the only combinations of
      return false;               // flags allowed by the C++ standard.
  }

  // fbp : TODO : set permissions !
  (void)permission; // currently unused    //*TY 02/26/2000 - added to suppress warning message
  _M_file = FOPEN(name, flags);

  if (_M_file) {
    file_no = fileno(_M_file);
  } else {
    return false;
  }

  // unset buffering immediately
  setbuf(_M_file, 0);

  _M_is_open = true;

  if (openmode & ios_base::ate) {
    if (FSEEK(_M_file, 0, SEEK_END) != 0)
      _M_is_open = false;
  }

  _M_file_id = file_no;
  _M_should_close = _M_is_open;
  _M_openmode = openmode;

  if (_M_is_open)
    _M_regular_file = _STLP_PRIV __is_regular_file(_M_file_id);

  return (_M_is_open != 0);
}


bool _Filebuf_base::_M_open(const char* name, ios_base::openmode openmode)
{
  // This doesn't really grant everyone in the world read/write
  // access.  On Unix, file-creation system calls always clear
  // bits that are set in the umask from the permissions flag.
  return this->_M_open(name, openmode, S_IRUSR | S_IWUSR | S_IRGRP |
                                       S_IWGRP | S_IROTH | S_IWOTH);
}

// Associated the filebuf with a file descriptor pointing to an already-
// open file.  Mode is set to be consistent with the way that the file
// was opened.
bool _Filebuf_base::_M_open( int file_no, ios_base::openmode )
{
  if (_M_is_open || file_no < 0)
    return false;

  struct STAT buf;
  if (FSTAT(file_no, &buf) != 0)
    return false;
  int mode = buf.st_mode;

  switch ( mode & (S_IWRITE | S_IREAD) ) {
    case S_IREAD:
      _M_openmode = ios_base::in;
      break;
    case S_IWRITE:
      _M_openmode = ios_base::out;
      break;
    case (S_IWRITE | S_IREAD):
      _M_openmode = ios_base::in | ios_base::out;
      break;
    default:
      return false;
  }
  _M_file_id = file_no;
  _M_is_open = true;
  _M_should_close = false;
  _M_regular_file = _STLP_PRIV __is_regular_file(_M_file_id);
  return true;
}

bool _Filebuf_base::_M_close()
{
  if (!_M_is_open)
    return false;

  bool ok = _M_should_close ? (fclose(_M_file) == 0) : true;

  _M_is_open = _M_should_close = false;
  _M_openmode = 0;
  return ok;
}

// Read up to n characters into a buffer.  Return value is number of
// characters read.
ptrdiff_t _Filebuf_base::_M_read(char* buf, ptrdiff_t n) {
  return fread(buf, 1, n, _M_file);
}

// Write n characters from a buffer.  Return value: true if we managed
// to write the entire buffer, false if we didn't.
bool _Filebuf_base::_M_write(char* buf, ptrdiff_t n)
{
  for (;;) {
    ptrdiff_t written = fwrite(buf, 1, n, _M_file);

    if (n == written) {
      return true;
    }

    if (written > 0 && written < n) {
      n -= written;
      buf += written;
    } else {
      return false;
    }
  }
}

// Wrapper for lseek or the like.
streamoff _Filebuf_base::_M_seek(streamoff offset, ios_base::seekdir dir)
{
  int whence;

  switch ( dir ) {
    case ios_base::beg:
      if (offset < 0 /* || offset > _M_file_size() */ )
        return streamoff(-1);
      whence = SEEK_SET;
      break;
    case ios_base::cur:
      whence = SEEK_CUR;
      break;
    case ios_base::end:
      if (/* offset > 0 || */  -offset > _M_file_size() )
        return streamoff(-1);
      whence = SEEK_END;
      break;
    default:
      return streamoff(-1);
  }

  if ( FSEEK(_M_file, offset, whence) == 0 ) {
    return FTELL(_M_file);
  }

  return streamoff(-1);
}


// Attempts to memory-map len bytes of the current file, starting
// at position offset.  Precondition: offset is a multiple of the
// page size.  Postcondition: return value is a null pointer if the
// memory mapping failed.  Otherwise the return value is a pointer to
// the memory-mapped file and the file position is set to offset.
void *_Filebuf_base::_M_mmap(streamoff, streamoff )
{
  return 0;
}

void _Filebuf_base::_M_unmap(void*, streamoff)
{
  // precondition : there is a valid mapping at the moment
}

// fbp : let us map 1 MB maximum, just be sure not to trash VM
#define MMAP_CHUNK 0x100000L

_Underflow< char, char_traits<char> >::int_type _STLP_CALL
_Underflow< char, char_traits<char> >::_M_doit(basic_filebuf<char, char_traits<char> >* __this)
{
  typedef char_traits<char> traits_type;
  typedef traits_type::int_type int_type;

  if (!__this->_M_in_input_mode) {
    if (!__this->_M_switch_to_input_mode())
      return traits_type::eof();
  }
  else if (__this->_M_in_putback_mode) {
    __this->_M_exit_putback_mode();
    if (__this->gptr() != __this->egptr()) {
      int_type __c = traits_type::to_int_type(*__this->gptr());
      return __c;
    }
  }

  // If it's a disk file, and if the internal and external character
  // sequences are guaranteed to be identical, then try to use memory
  // mapped I/O.  Otherwise, revert to ordinary read.
  if (__this->_M_base.__regular_file()
      && __this->_M_always_noconv
      && __this->_M_base._M_in_binary_mode()) {
    // If we've mmapped part of the file already, then unmap it.
    if (__this->_M_mmap_base) {
      __this->_M_base._M_unmap(__this->_M_mmap_base, __this->_M_mmap_len);
      __this->_M_mmap_base = 0;
      __this->_M_mmap_len = 0;
    }

    // Determine the position where we start mapping.  It has to be
    // a multiple of the page size.
    streamoff __cur = __this->_M_base._M_seek(0, ios_base::cur);
    streamoff __size = __this->_M_base._M_file_size();
    if (__size > 0 && __cur >= 0 && __cur < __size) {
      streamoff __offset = (__cur / __this->_M_base.__page_size()) * __this->_M_base.__page_size();
      streamoff __remainder = __cur - __offset;

      __this->_M_mmap_len = __size - __offset;

      if (__this->_M_mmap_len > MMAP_CHUNK)
        __this->_M_mmap_len = MMAP_CHUNK;

      if ((__this->_M_mmap_base = __this->_M_base._M_mmap(__offset, __this->_M_mmap_len)) != 0) {
        __this->setg((char*) __this->_M_mmap_base,
                     (char*) __this->_M_mmap_base + __STATIC_CAST(ptrdiff_t, __remainder),
                     (char*) __this->_M_mmap_base + __STATIC_CAST(ptrdiff_t, __this->_M_mmap_len));
        return traits_type::to_int_type(*__this->gptr());
      }
      else
        __this->_M_mmap_len = 0;
    }
  }

  return __this->_M_underflow_aux();
}

_STLP_END_NAMESPACE
