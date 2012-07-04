#ifndef _FILE_APPENDER_H_
#define _FILE_APPENDER_H_

#pragma once
// $Id: FileAppender.h,v 1.1.2.1 2008/06/06 17:25:54 alexey Exp $
#include <stdio.h>
#ifndef _WIN32
#include <time.h>
#endif
#include "Logger.h"
#include <string>

LOGGER_NS_BEGIN

template <class MUTEX> class FileAppender : public Appender {
private:
  MUTEX mutex;
  string m_filename;
  const char* m_mode;
  FILE* m_file;

protected:

  void write(const Logger::Level& level, const char* format, va_list args) {
    typename LockBase<MUTEX> lock(mutex);
    if (m_file == NULL) {
      m_file = fopen(m_filename.c_str(), m_mode);
      if (m_file != NULL) {
        if (ftell(m_file) > 0) {
          fprintf(m_file, "\n\n");
        }
      }
    }
    if (m_file != NULL) {
#ifdef _WIN32
      SYSTEMTIME st;
      GetLocalTime(&st);
      fprintf(m_file, "%04d-%02d-%02d %02d:%02d:%02d.%03d ", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
      fprintf(m_file, "%08x ", GetCurrentThreadId());
#else
      time_t timeValue = time(NULL);
      struct tm t;      
      if (localtime_r(&timeValue, &t) != NULL) {
        fprintf(m_file, "%04d-%02d-%02d %02d:%02d:%02d ", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
      }
#endif
      fprintf(m_file, "%-5s ", level.c_str());
      vfprintf(m_file, format, args);
      fprintf(m_file, "\n");
      fflush(m_file);
    }
  }

public:

  FileAppender(const string& filename): m_filename(filename), m_mode("wt"), m_file(NULL) { }

  FileAppender(const string& filename, const char* mode): m_filename(filename), m_mode(mode), m_file(NULL) { }

  virtual ~FileAppender() {
    if (m_file != NULL) {
      fclose(m_file);
      m_file = NULL;
    }
  }
};

LOGGER_NS_END

#endif /* _FILE_APPENDER_H_ */
