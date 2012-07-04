#include "stdafx.h"
#include "IniFile.h"

PeersIniFile::PeersIniFile() {
  TCHAR appPath[MAX_PATH];
  GetModuleFileName(NULL, appPath, MAX_PATH);
  PTSTR ptr = _tcsrchr(appPath, '.');
  if (ptr) {
    ++ptr;
    _tcscpy(ptr, _T("ini"));
    SetFilename(appPath);
  }
  else {
    SetFilename(_T("Peers.ini"));
  }
}
