#include "../client/stdinc.h"
#include "../client/DCPlusPlus.h"
#include "FileUtils.h"

void FileUtils::ensureDirectoryA(const string& file) throw() {
  string::size_type start = file.find_first_of("\\/");
  if (start != string::npos) {
    start++;
    while ((start = file.find_first_of("\\/", start)) != string::npos) {
      ::CreateDirectoryA(file.substr(0, start+1).c_str(), NULL);
      start++;
    }
  }
}

void FileUtils::ensureDirectory(const tstring& file) throw() {
  tstring::size_type start = file.find_first_of(_T("\\/"));
  if (start != string::npos) {
    start++;
    while ((start = file.find_first_of(_T("\\/"), start)) != string::npos) {
      ::CreateDirectory(file.substr(0, start+1).c_str(), NULL);
      start++;
    }
  }
}
