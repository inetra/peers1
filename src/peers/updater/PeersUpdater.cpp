#include "../../client/stdinc.h"
#include "../../client/DCPlusPlus.h"
#include "../../client/CriticalSection.h"
#include "../../client/File.h"
#include "../../utils/SystemUtils.h"
#include "../../utils/FileUtils.h"
#include <Tlhelp32.h>
#include "../AutoUpdate.h"
#include "../unzip/ZipInputStream.h"
#include "../../logger/logger.h"
#include "../../logger/FileAppender.h"

#define INVALID_PROCESS_ID ((DWORD)-1)
#define WRITEBUFFERSIZE 4096

//======================================================================
/* возвращает идентификатор родительского процесса, INVALID_PROCESS_ID при ошибке */
//======================================================================
DWORD findParentProcessID() {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return INVALID_PROCESS_ID;
  }
  DWORD currentProcessId = GetCurrentProcessId();
  DWORD parentId = INVALID_PROCESS_ID;
  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);
  if (Process32First(snapshot, &entry)) {
    do {
      if (entry.th32ProcessID == currentProcessId) {
	parentId = entry.th32ParentProcessID;
	break;
      }
    } while (Process32Next(snapshot, &entry));
  }
  CloseHandle(snapshot);
  return parentId;
}

//======================================================================
/* находит родительский процесс и открывает его handle, возвращает INVALID_HANDLE_VALUE при ошибке */
//======================================================================
HANDLE openParentProcess() {
  DWORD parentId = findParentProcessID();
  if (parentId == INVALID_PROCESS_ID) {
    return INVALID_HANDLE_VALUE;
  }
  return OpenProcess(SYNCHRONIZE, FALSE, parentId);
}

class PeersUpdaterUnzip : public ZipInputStream {
private:
	StringPairList m_pairs;
	string m_directory;

	bool toLocalTime(FILETIME& fileTime) {
		SYSTEMTIME systemTime;
		if (!FileTimeToSystemTime(&fileTime, &systemTime)) {
			return false;
		}
		SYSTEMTIME localTime;
		if (!SystemTimeToTzSpecificLocalTime(NULL, &systemTime, &localTime)) {
			return false;
		}
		if (!SystemTimeToFileTime(&localTime, &fileTime)) {
			return false;
		}
		return true;
	}

	uint64_t toInt64(const FILETIME& fileTime) {
		ULARGE_INTEGER value;
		value.LowPart = fileTime.dwLowDateTime;
		value.HighPart = fileTime.dwHighDateTime;
		return value.QuadPart;
	}

	bool isModified(HANDLE file, const unz_file_info& fileInfo) {
		// Hotfix by mirror@inetra.ru when upgrading from 602 to 603 revision
		// TODO: check md5sum, not filesize or date
		return true;
		DWORD fileSize = GetFileSize(file, NULL);
		if (fileSize == INVALID_FILE_SIZE) {
			debug_log("  fileSize == INVALID_FILE_SIZE");
			return true;
		}
		if (fileSize != fileInfo.uncompressed_size) {
			debug_log("  size mismatch (%d vs %d)", (int) fileSize, (int) fileInfo.uncompressed_size);
			return true;
		}
		FILETIME lastModifyTime;
		if (!GetFileTime(file, NULL, NULL, &lastModifyTime)) {
			debug_log("  !GetFileTime");
			return true;
		}
		if (!toLocalTime(lastModifyTime)) {
			debug_log("  error in toLocalTime()");
			return true;
		}
		FILETIME newModifyTime;
		DosDateTimeToFileTime(HIWORD(fileInfo.dosDate), LOWORD(fileInfo.dosDate), &newModifyTime);
		// FILETIME uses 100-nanosecond intervals, so 10 * 1000 * 1000
		// 2 second is DosDateTime & FAT precision
		if (Util::abs_64(toInt64(lastModifyTime) - toInt64(newModifyTime)) < 2 * 10 * 1000 * 1000) {
			debug_log("  time differs %I64x vs %I64x", lastModifyTime, newModifyTime);
			return true;
		}
		// TODO check crc32
		return false;
	}

protected:
	virtual void processEntry(const char* entryName, const unz_file_info& fileInfo) {
		debug_log("processEntry %s", entryName);
		if (strcmpi(entryName, PEERS_UPDATER_EXE) == 0) {
			debug_log("  skip self");
			skipEntry();
			return;
		}
		size_t nameLen = strlen(entryName);
		if (nameLen > 0 && entryName[nameLen-1] == '/') {
			debug_log("  skip directory \"%s\"", entryName);
			skipEntry();
			return;
		}
		string normalPath = m_directory + entryName;
		HANDLE in = ::CreateFileA(normalPath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (in != INVALID_HANDLE_VALUE) {
			bool modified = isModified(in, fileInfo);
			CloseHandle(in);
			if (!modified) {
				info_log("  skip \"%s\" not modified", entryName);
				skipEntry();
				return;
			}
		}
		FileUtils::ensureDirectoryA(normalPath);
		string workPath = normalPath + ".new";
		HANDLE out = ::CreateFileA(workPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (out == INVALID_HANDLE_VALUE) {
			throw Exception("Error opening " + workPath + ": " + SystemUtils::describeError(GetLastError()));
		}
		try {
			for (;;) {
				BYTE buffer[WRITEBUFFERSIZE];
				size_t count = readCurrentFile(buffer, WRITEBUFFERSIZE);
				if (count == 0) {
					break;
				}
				DWORD written;
				if (!::WriteFile(out, buffer, (DWORD) count, &written, NULL)) {
					throw Exception("error writing " + workPath);
				}
				if (count != written) {
					throw Exception("error writing " + workPath);
				}
			}
		}
		catch (Exception& e) {
			error_log("Error %s", e.getError().c_str());
			CloseHandle(out);
			DeleteFileA(workPath.c_str());
			throw e;
		}
		FILETIME lastModTime;
		DosDateTimeToFileTime(HIWORD(fileInfo.dosDate), LOWORD(fileInfo.dosDate), &lastModTime);
		SetFileTime(out, NULL, NULL, &lastModTime);
		CloseHandle(out);
		m_pairs.push_back(make_pair(normalPath, workPath));
	}

public:

	PeersUpdaterUnzip(const string& inputFile, const string& directory): ZipInputStream(inputFile.c_str()), m_directory(directory) {
	}

	void renameTempFiles() {
		for (StringPairIter i = m_pairs.begin(); i != m_pairs.end(); ++i) {
			debug_log("rename %s to %s", i->second.c_str(), i->first.c_str());
			for (int j = 0; j < 10; ++j) {
				if (MoveFileExA(i->second.c_str(), i->first.c_str(), MOVEFILE_REPLACE_EXISTING)) {
					break;
				}
				error_log("rename error: %s", SystemUtils::describeError(GetLastError()).c_str());
				Sleep(1000);
			}
		}
	}

	void deleteTempFiles() {
		for (StringPairIter i = m_pairs.begin(); i != m_pairs.end(); ++i) {
			if (!DeleteFileA(i->second.c_str())) {
				error_log("delete error: %s", SystemUtils::describeError(GetLastError()).c_str());
			}
		}
	}
};

//======================================================================
/* открытие и запуск распаковки */
//======================================================================
void unzip(const string& zipFileName, const string& directory) {
  debug_log("Opening %s", zipFileName.c_str());
  try {
    PeersUpdaterUnzip zip(zipFileName, directory);
    try {
      zip.processEntries();
      zip.renameTempFiles();
    }
    catch (Exception& e) {
      zip.deleteTempFiles();
      throw e;
    }
  }
  catch (Exception& e) {
    error_log("Error unzipping files: %s", e.getError().c_str());
  }
  if (!DeleteFileA(zipFileName.c_str())) {
    error_log("Error deleting %s: %s", zipFileName.c_str(), SystemUtils::describeError(GetLastError()).c_str());
  }
}

//======================================================================
// возвращает каталог приложения, включая конечный слеш
//======================================================================
string getApplicationDirectory() {
  char path[MAX_PATH];
  GetModuleFileNameA(NULL, path, MAX_PATH);
  char *ptr = strrchr(path, PATH_SEPARATOR);
  if (ptr == NULL) {
    return string();
  }
  else {
    ++ptr;
    return string(path, ptr - path);
  }
}

static void registerLogger() {
	tstring logPath = SystemUtils::getAppDataFolder() + _T("update.log");
	char path[1024];
	int pathLen = WideCharToMultiByte(CP_ACP, 0, logPath.c_str(), (int) logPath.length(), path, (int) COUNTOF(path), NULL, NULL);
	Logger::Repository::registerAppender(Logger::Repository::DEFAULT_CATEGORY, new Logger::FileAppender<CriticalSection>(string(path, pathLen), "at"));
}

static bool isValidDirectory(const string& directory) {
	string d = directory;
	if (!d.empty() && d[d.length() - 1] == '\\') {
		d = d.substr(0, d.length() - 1);
	}
	return true;
	//TODO: File::exists(Text::acpToUtf8(d));
}

//======================================================================
// Главная точка входа
//======================================================================
#ifndef _DEBUG
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
#else
int _cdecl main(int argc, char *argv[]) {
#endif
  registerLogger();
  HANDLE hParent = openParentProcess();
  if (hParent != INVALID_HANDLE_VALUE) {
    debug_log("Waiting parent to terminate");
#ifdef _DEBUG
    DWORD timeout = 2000;
#else
    DWORD timeout = 60000;
#endif
    if (WaitForSingleObject(hParent, timeout) != WAIT_OBJECT_0) {
      debug_log("Error waiting parent");
    }
  }
  else {
	  debug_log("Could not open parent process - just sleep 1 sec.");
	  Sleep(1000);
  }

#ifndef _DEBUG
  char directoryBuf[1024];
  int resultSize = WideCharToMultiByte(CP_ACP, 0, lpstrCmdLine, -1, directoryBuf, COUNTOF(directoryBuf), NULL, NULL);
  string directory = string(directoryBuf);
  debug_log("CommandLine=[%s]", directory.c_str());
#else
  for (int i = 0; i <argc; ++i) {
	  debug_log("argv[%d]=[%s]", i, argv[i]);
  }
  string directory;
  if (argc >= 2) {
	  directory = argv[1];
  }
#endif
  // remove surrounded quotes if any
  if (!directory.empty() && directory[0] == '"') {
	  directory = directory.substr(1);
  }
  if (!directory.empty() && directory[directory.length() - 1] == '"') {
	  directory = directory.substr(0, directory.length() - 1);
  }
  debug_log("directory=[%s]", directory.c_str());

  if (directory.empty()) {
	  error_log("Installation directory is not specified");
	  return 0;
  }
  if (!isValidDirectory(directory)) {
	  error_log("Installation directory \"%s\" does not exist", directory.c_str());
	  return 0;
  }

  unzip(getApplicationDirectory() + PEERS_UPDATE_ZIP, directory);
  string command = directory + PEERS_EXE + " " + PEERS_OPTION_NO_UPDATE;
  STARTUPINFOA si;
  memset(&si, 0, sizeof(STARTUPINFOA));
  si.cb = sizeof(STARTUPINFOA);
  si.dwFlags = STARTF_FORCEOFFFEEDBACK;
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));
  PSTR commandBuffer = (char*) malloc(command.length() + 1);
  if (commandBuffer) {
    strcpy(commandBuffer, command.c_str());
    BOOL result = CreateProcessA(NULL, commandBuffer, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
	if (!result) {
		error_log("Error executing %s: %s", commandBuffer, SystemUtils::describeError(GetLastError()).c_str());
	}
	else {
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
	}
    free(commandBuffer);
  }
  return 0;
}
