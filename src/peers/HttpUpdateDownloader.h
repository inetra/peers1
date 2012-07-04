#pragma once

#include "../client/HttpConnection.h"
#include "../md5/MD5Digest.h"

class HttpUpdateDownloader;

class HttpUpdateDownloaderListener {
public:
  virtual void onDownloadComplete(HttpUpdateDownloader* downloader) throw() = 0;
};

class HttpUpdateDownloader : protected HttpConnectionListener {
private:
  HttpConnection connection;
  HttpUpdateDownloaderListener* m_listener;
  HANDLE file;
  bool fileError;
  tstring targetPath;
  bool m_hidden;
  int m_mode;
  size_t m_currentSize;
  size_t m_fileSize;
  string m_fileMD5;
  MD5Digest m_digest;
  tstring getTempFile() const {
    return targetPath + _T(".tmp");
  }
protected:
  virtual void on(Data, HttpConnection*, const uint8_t*, size_t) throw();
  virtual void on(Complete, HttpConnection*, const string&) throw();
public:
  HttpUpdateDownloader(bool hidden, int mode, const tstring& aTargetPath, const size_t fileSize, const string& fileMD5);
  ~HttpUpdateDownloader(void);
  void downloadFile(const string& url);	
  void setListener(HttpUpdateDownloaderListener* listener) {
    m_listener = listener;
  }
  int getMode() const {
	  return m_mode;
  }
};
