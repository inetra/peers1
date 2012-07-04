#include "stdafx.h"
#include "AutoUpdate.h"
#include "../md5/MD5Digest.h"
#include "../../utils/SystemUtils.h"
#include "../../utils/FileUtils.h"
#include "unzip/ZipInputStream.h"

#define WRITEBUFFERSIZE 4096

/* возвращает путь, в который нужно скачивать обновления */
tstring AutoUpdate::getUpdateTargetPath() {
	return SystemUtils::getAppDataFolder() + _T(PEERS_UPDATE_ZIP);
}

/* возвращает путь к каталогу приложения включая последний слеш */
tstring AutoUpdate::getApplicationDirectory() {
  TCHAR path[MAX_PATH];
  GetModuleFileName(NULL, path, MAX_PATH);
  TCHAR *ptr = _tcsrchr(path, '\\');
  if (ptr) {
    *(ptr + 1) = 0;
    return path;
  }
  else {
    return Util::emptyStringT;
  }
}


class UpdaterExractor : public ZipInputStream {
private:
	string m_directory;

	bool isModified(HANDLE file, const unz_file_info& fileInfo) {
		DWORD fileSize = GetFileSize(file, NULL);
		if (fileSize == INVALID_FILE_SIZE) {
			//debug_log("fileSize == INVALID_FILE_SIZE");
			return true;
		}
		if (fileSize != fileInfo.uncompressed_size) {
			//debug_log("size mismatch (%d vs %d)", (int) fileSize, (int) fileInfo.uncompressed_size);
			return true;
		}
		FILETIME lastModifyTime;
		if (!GetFileTime(file, NULL, NULL, &lastModifyTime)) {
			//debug_log("!GetFileTime");
			return true;
		}
		FILETIME newModifyTime;
		DosDateTimeToFileTime(HIWORD(fileInfo.dosDate), LOWORD(fileInfo.dosDate), &newModifyTime);
		if (CompareFileTime(&lastModifyTime, &newModifyTime) != 0) {
			//debug_log("time differs");
			return true;
		}
		// TODO check crc32
		return false;
	}

protected:
	virtual void processEntry(const char* entryName, const unz_file_info& fileInfo) {
		//debug_log("processEntry %s", entryName);
		if (stricmp(entryName, PEERS_UPDATER_EXE) != 0) {
			//debug_log("skip directory \"%s\"", entryName);
			skipEntry();
			return;
		}
		BYTE buffer[WRITEBUFFERSIZE];
		string normalPath = m_directory + entryName;
		HANDLE in = ::CreateFileA(normalPath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (in != INVALID_HANDLE_VALUE) {
			bool modified = isModified(in, fileInfo);
			CloseHandle(in);
			if (!modified) {
				//info_log("skip \"%s\" not modified", entryName);
				skipEntry();
				return;
			}
		}
		FileUtils::ensureDirectoryA(normalPath);
		HANDLE out = ::CreateFileA(normalPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (out == INVALID_HANDLE_VALUE) {
			throw Exception("Error opening " + normalPath + ": " + SystemUtils::describeError(GetLastError()));
		}
		try {
			for (;;) {
				size_t count = readCurrentFile(buffer, WRITEBUFFERSIZE);
				if (count == 0) {
					break;
				}
				DWORD written;
				if (!::WriteFile(out, buffer, (DWORD) count, &written, NULL)) {
					throw Exception("error writing " + normalPath);
				}
				if (count != written) {
					throw Exception("error writing " + normalPath);
				}
			}
		}
		catch (Exception& e) {
			//error_log("Error %s", e.getError().c_str());
			CloseHandle(out);
			DeleteFileA(normalPath.c_str());
			throw e;
		}
		FILETIME lastModTime;
		DosDateTimeToFileTime(HIWORD(fileInfo.dosDate), LOWORD(fileInfo.dosDate), &lastModTime);
		SetFileTime(out, NULL, NULL, &lastModTime);
		CloseHandle(out);
	}

public:

	UpdaterExractor(const string& inputFile, const string& directory): ZipInputStream(inputFile.c_str()), m_directory(directory) {
	}

};

/* выполнение автообновления. возвращает true, если процесс удалось запустить */
bool AutoUpdate::execute() {
	tstring updatePath = getUpdateTargetPath();
	tstring appData = SystemUtils::getAppDataFolder();
	try {
		UpdaterExractor extractor(Text::wideToAcp(updatePath), Text::wideToAcp(appData));
		extractor.processEntries();
	}
	catch (Exception& e) {
		//error_log("Error unzipping files: %s", e.getError().c_str());
		return false;
	}

  tstring command = appData + _T(PEERS_UPDATER_EXE);
  tstring appDir = getApplicationDirectory();
  HINSTANCE instance = ShellExecute(NULL, _T("open"), command.c_str(), appDir.c_str(), NULL, SW_SHOWNORMAL);
  if ((int) instance > 32) {
    return true;
  }
  else {
#ifdef _DEBUG
	  DWORD lastError = GetLastError();
	  debugTrace("Error starting updater %d: %s\n", lastError, SystemUtils::describeError(lastError));
#endif
	  return false;
  }
}

/* проверяет указанный файл - что он существует, у него указанные размер и md5 */
bool AutoUpdate::checkFile(const tstring& fileName, long fileSize, const string& md5) {
  try {
    File file(Text::fromT(fileName), File::READ, File::OPEN);
    if (file.getSize() != fileSize) {
#ifdef _DEBUG
      debugTrace("File size mismatches (%l actual vs %l expected)\n", file.getSize(), fileSize);
#endif
      return false;
    }
    MD5Digest digest;
    BYTE buffer[4096];
    for (;;) {
      size_t bufferSize = sizeof(buffer);
      if (file.read(buffer, bufferSize) == 0) {
	break;
      }
      digest.update(buffer, bufferSize);
    }
	const string fileMD5 = digest.digestAsString();
#ifdef _DEBUG
    debugTrace("MD5: actual %s, expected %s\n", fileMD5.c_str(), md5.c_str());
#endif
	return Util::stricmp(fileMD5, md5) == 0;
  }
  catch (FileException &) {
    return false;
  }
  return true;
}

#ifdef _DEBUG

#define PEERS_INI_SECTION_AUTOUPDATE "autoupdate"

/* нужно ли выполнять проверку обновлений при старте */
bool AutoUpdate::allowOnStart() {
  PeersIniFile ini;
  bool result = true;
  ini.GetBool(_T(PEERS_INI_SECTION_AUTOUPDATE), _T("start"), result);
  return result;
}

/* нужно ли выполнять периодическую проверку обновлений */
bool AutoUpdate::allowPeriodic() {
  PeersIniFile ini;
  bool result = true;
  ini.GetBool(_T(PEERS_INI_SECTION_AUTOUPDATE), _T("periodic"), result);
  return result;
}

/* возвращает интервал выполнения периодической проверки обновлений, в секундах */
int AutoUpdate::getUpdateCheckInterval() {
  PeersIniFile ini;
  int interval = 60 * 60;
  ini.GetInt(_T(PEERS_INI_SECTION_AUTOUPDATE), _T("interval"), interval);
  return max(interval, 20);
}

#endif
