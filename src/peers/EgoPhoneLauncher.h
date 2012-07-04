#ifndef _EGO_PHONE_LAUNCHER_H_
#define _EGO_PHONE_LAUNCHER_H_
#pragma once

class EgoPhoneLauncher {
private:
  HWND m_parentWnd;
  // returns true if started
  bool startIfInstalled();
public:
  EgoPhoneLauncher(HWND parentWnd): m_parentWnd(parentWnd) { }
  void execute();
};

#endif /* _EGO_PHONE_LAUNCHER_H_ */
