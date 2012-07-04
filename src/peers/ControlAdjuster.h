#pragma once

class ControlAdjuster {
private:
#if (_WIN32_WINNT >= 0x0501)
  static bool isWindows2000();
#endif
public:
  enum { HEADER_ICON_HEIGHT = 32 };
  static SIZE adjustEditSize(CEdit& edit, int length);
  static int getComboBoxHeight(CComboBox &combo);
  static SIZE adjustComboBoxSize(CComboBox &combo);
  static SIZE adjustStaticSize(CStatic s);
  static SIZE adjustStaticSize(CStatic s, bool wordWrap);
  static int adjustHeaderHeight(HWND hwnd);
  static POINT getWindowOrigin(HWND hwnd);
  /*
  static void resizeWindow(HWND hwnd, const SIZE& windowSize);
  static void moveWindow(HWND hwnd, int x, int y);
  */
#if (_WIN32_WINNT >= 0x0501)
  void static inline fixToolInfoSize(TOOLINFO &ti) {
    if (isWindows2000()) {
      ti.cbSize -= sizeof(void*);
    }
  }
#else
  void static inline fixToolInfoSize(TOOLINFO &ti) { /* empty */ }
#endif
};
