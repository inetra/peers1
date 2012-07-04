#include "stdafx.h"
#include "HttpUpdateDownloader.h"
#include "PeersUtils.h"
#include "../client/File.h"
#include "../utils/SystemUtils.h"
#include "../utils/FileUtils.h"

HttpUpdateDownloader::HttpUpdateDownloader(bool hidden, int mode, const tstring &aTargetPath, const size_t fileSize, const string& fileMD5):
connection(PeersUtils::getUserAgent()),
targetPath(aTargetPath),
m_hidden(hidden),
m_mode(mode),
m_currentSize(0),
m_fileSize(fileSize),
m_fileMD5(fileMD5),
file(INVALID_HANDLE_VALUE),
fileError(false)
{
  connection.addListener(this);
}

void HttpUpdateDownloader::downloadFile(const string& url) {
  connection.downloadFile(url);
}

void HttpUpdateDownloader::on(Data, HttpConnection*, const uint8_t* buffer, size_t length) throw(){
#ifdef _DEBUG
  debugTrace("onData: %d bytes\n", length);
#endif
  if (file == INVALID_HANDLE_VALUE) {
    if (fileError) {
      // TODO лучше при ошибке просто закрыть сокет.
      return;
    }
	FileUtils::ensureDirectory(targetPath);
	DWORD flags = FILE_ATTRIBUTE_NOT_CONTENT_INDEXED;
	if (m_hidden) {
		flags |= FILE_ATTRIBUTE_HIDDEN;
	}
    file = CreateFile(getTempFile().c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, flags, NULL);
    if (file == INVALID_HANDLE_VALUE) {
#ifdef _DEBUG
		DWORD errorCode = GetLastError();
		debugTrace("Error %d creating file\n", SystemUtils::describeError(errorCode).c_str());
#endif
      fileError = true;
      return;
    }
  }
  DWORD written;
  if (!WriteFile(file, buffer, length, &written, NULL) || written != length) {
#ifdef _DEBUG
    debugTrace("Error %d writing file\n", GetLastError());
#endif
    CloseHandle(file);
    DeleteFile(targetPath.c_str());
    fileError = true;
    return;
  }
  m_currentSize += length;
  m_digest.update(buffer, length);
}

void HttpUpdateDownloader::on(Complete, HttpConnection*, const string&) throw() {
#ifdef _DEBUG
	debugTrace("on(ModeChange)\n");
#endif
	if (!fileError && file != INVALID_HANDLE_VALUE) {
		CloseHandle(file);
		if (m_currentSize != m_fileSize) {
			char buffer[2048];
			snprintf(buffer, sizeof(buffer), "Size mismatch %s (downloaded %d, expected %d)", Util::getFileName(Text::fromT(targetPath)).c_str(), m_currentSize, m_fileSize);
			LOG_MESSAGE(buffer);
			DeleteFile(getTempFile().c_str());
			return;
		}
		const string currentMD5 = m_digest.digestAsString();
		if (Util::stricmp(currentMD5, m_fileMD5) != 0) {
			char buffer[2048];
			snprintf(buffer, sizeof(buffer), "MD5 mismatch %s (downloaded %s, expected %s)", Util::getFileName(Text::fromT(targetPath)).c_str(), currentMD5.c_str(), m_fileMD5.c_str());
			LOG_MESSAGE(buffer);
			DeleteFile(getTempFile().c_str());
			return;
		}
		LOG_MESSAGE("Successfully downloaded " + Text::fromT(targetPath));
		try {
			File::atomicRename(getTempFile(), targetPath);
			// файл скачался
			if (m_listener != NULL) {
				m_listener->onDownloadComplete(this);
			}
		}
		catch (const Exception& e) {
			LOG_MESSAGE("Error updating " + Text::fromT(targetPath) + ": " + e.getError());
		}
	}
}

HttpUpdateDownloader::~HttpUpdateDownloader(void) {
}
