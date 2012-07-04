#ifndef _INIFILE_H_
#define _INIFILE_H_
#pragma once

#if defined(DC_PLUS_PLUS_H)
#error IniFile.h should be included before DCPlusPlus.h
#endif
#include <atlbase.h>
#include <atlwinmisc.h>

class PeersIniFile : public CIniFile {
public:
  PeersIniFile();
};

#endif /* _INIFILE_H_ */
