#pragma once
#include "../client/peers/ClientLifeCycle.h"

class SplashWindow : public CWindowImpl<SplashWindow>, private ProgressCallback {
private:
  HBITMAP bitmap;
  int screenWidth;
  int screenHeight;
  int bitmapWidth;
  int bitmapHeight;
  tstring g_sSplashText;
  tstring g_sTitle;
protected:
  // ProgressCallback
  virtual void showMessage(const tstring& value);
public:
  SplashWindow(short resourceId);
  virtual ~SplashWindow(void);
  void init();

public:
  BEGIN_MSG_MAP(SplashWindow)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_CREATE, OnCreate);
  END_MSG_MAP()

  LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

  void setSplashText(const tstring& value);

  ProgressCallback* getCallback() const { return (ProgressCallback*) this; }

};
