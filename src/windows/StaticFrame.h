#pragma once

template<class T, int title, int ID = -1>
class StaticFrame {
private:
  static T* frame;
public:
	static T* getInstance() { return frame; }

  virtual ~StaticFrame() { frame = NULL; }

  static void openWindow(bool closeIfActive = true) {
    if (frame == NULL) {
      frame = new T();
      frame->CreateEx(WinUtil::mdiClient, frame->rcDefault, CTSTRING_I(ResourceManager::Strings(title)));
      WinUtil::setButtonPressed(ID, true);
    } 
    else {
      HWND hWnd = frame->m_hWnd;
      if (WinUtil::MDIGetActive() == hWnd) {
        if (closeIfActive) {
          ::PostMessage(hWnd, WM_CLOSE, NULL, NULL);
          return;
        }
      } 
      else {
        WinUtil::MDIActivate(hWnd);
        WinUtil::setButtonPressed(ID, true);
      }
      if (::IsIconic(hWnd)) {
        ::ShowWindow(hWnd, SW_MAXIMIZE);
      }
    }
  }
  // !SMT!-S
  static void closeWindow() {
    if (frame) {
      ::PostMessage(frame->m_hWnd, WM_CLOSE, NULL, NULL);
    }
  }
};

template<class T, int title, int ID> T* StaticFrame<T, title, ID>::frame = NULL;
