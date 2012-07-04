#ifndef _ZIP_INPUT_STREAM_H_
#define _ZIP_INPUT_STREAM_H_
#pragma once

#define STRICTUNZIP
#include "unzip.h"
#include "../../client/Exception.h"

class ZipException : public Exception {
private:
  int m_code;
public:
  ZipException(const string& aError) throw(): Exception(aError), m_code(0) { }
  ZipException(const string& aError, int code) throw(): Exception(aError), m_code(code) { }
  virtual ~ZipException() throw() { }
  int getCode() const { return m_code; }
};

class ZipInputStream {
private:
  unzFile m_zipFile;
protected:
  size_t readCurrentFile(void* buffer, size_t bufferSize);
  void skipEntry();
  virtual void processEntry(const char* entryName, const unz_file_info& fileInfo);
public:
  ZipInputStream(): m_zipFile(NULL) {
  }

  ZipInputStream(const string& filename): m_zipFile(NULL) {
    if (!open(filename.c_str())) {
      throw ZipException("Error opening file " + filename);
    }
  }

  virtual ~ZipInputStream() {
    close();
  }

  bool open(const char* filename);

  void close() {
    if (m_zipFile != NULL) {
      unzClose(m_zipFile);
      m_zipFile = NULL;
    }
  }

  void processEntries();

};

#endif /* _ZIP_INPUT_STREAM_H_ */
