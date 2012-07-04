#include "stdafx.h"
#include "SplashWindow.h"
#include "../peers/PeersVersion.h"

SplashWindow::SplashWindow(short resourceId) {
  bitmapWidth = 0;
  bitmapHeight = 0;
  screenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
  screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);
  bitmap = (HBITMAP) LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(resourceId), IMAGE_BITMAP, 0, 0, LR_SHARED);
  if (bitmap) {
    BITMAP bmpInfo;
    if (GetObject(bitmap, sizeof(BITMAP), &bmpInfo)) {
      bitmapWidth = bmpInfo.bmWidth;
      bitmapHeight = bmpInfo.bmHeight;
    }
  }
  if (bitmapWidth == 0 || bitmapHeight == 0) {
    bitmapWidth = screenWidth / 2;
    bitmapHeight = screenHeight / 2;
  }
  g_sTitle = _T(VERSIONSTRING);
  // 0 = CPU check not performed yet (Auto detect)
  // 1 = No optimization
  // 2 = MMX
  // 3 = MMX2 for AMD Athlon/Duron and above (might also work on MMX2 (KATMAI) Intel machines)
  // 4 = SSE
  // 5 = SSE2 (only for Pentium 4 detection, the optimization used is SSE)
  switch (get_cpu_type()) {
  case 2:
    g_sTitle += _T(" (MMX)");
    break;
  case 3:
    g_sTitle += _T(" (AMD)");
    break;
  case 4:
  case 5:
    g_sTitle += _T(" (SSE)");
    break;
  }
}

SplashWindow::~SplashWindow(void) {
  DeleteObject(bitmap);
}

void SplashWindow::showMessage(const tstring& value) {
  setSplashText(_T("Загрузка: ") + value);
}

void SplashWindow::setSplashText(const tstring& value) {
  g_sSplashText = value;
  Invalidate();
  UpdateWindow();
#ifdef _DEBUG
  debugTrace("showMessage(%ws)\n", value.c_str());
  Sleep(1000);
#endif
}

void SplashWindow::init() {
  Create(GetDesktopWindow(), rcDefault, NULL, WS_POPUP | WS_VISIBLE | WS_EX_TOOLWINDOW);
  UpdateWindow();
}

LRESULT SplashWindow::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  SetWindowPos(NULL, (screenWidth - bitmapWidth) / 2 - 1, (screenHeight - bitmapHeight) / 2 - 1, bitmapWidth + 2, bitmapHeight + 2, SWP_SHOWWINDOW);
  return 0;
}

LRESULT SplashWindow::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);  
  RECT rc;
  GetClientRect(&rc);
  CPen pen(CreatePen(PS_SOLID, 1, RGB(0xF0, 0xF0, 0xF0)));
  HPEN oldPen = dc.SelectPen(pen);
  dc.MoveTo(rc.left, rc.top);
  dc.LineTo(rc.left, rc.bottom - 1);
  dc.LineTo(rc.right -1, rc.bottom - 1);
  dc.LineTo(rc.right - 1, rc.top);
  dc.LineTo(rc.left, rc.top);
  dc.SelectPen(oldPen);
  if (bitmap) {
#ifdef _DEBUG
    debugTrace("OnPaint - draw bitmap\n");
#endif
    CDC comp;
    comp.CreateCompatibleDC(dc);    
    comp.SelectBitmap(bitmap);
    dc.BitBlt(1, 1, bitmapWidth, bitmapHeight, comp, 0, 0, SRCCOPY);
  }
  LOGFONT logFont;
  GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(logFont), &logFont);
  lstrcpy(logFont.lfFaceName, TEXT("Tahoma"));
  logFont.lfHeight = -12;
  logFont.lfWeight = FW_REGULAR;
  CFont hFont(CreateFontIndirect(&logFont));
  HFONT oldFont = dc.SelectFont(hFont);
  dc.SetTextColor(RGB(0,0,0));
  InflateRect(&rc, -3, -3);
  dc.DrawText(g_sTitle.c_str(), g_sTitle.length(), &rc, DT_RIGHT);
  dc.DrawText(g_sSplashText.c_str(), g_sSplashText.length(), &rc, DT_LEFT | DT_BOTTOM | DT_SINGLELINE);
  dc.SelectFont(oldFont);
  return 0;
}
