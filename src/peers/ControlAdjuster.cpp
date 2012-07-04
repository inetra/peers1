#include "stdafx.h"
#include "ControlAdjuster.h"
#include "CaptionFont.h"

SIZE ControlAdjuster::adjustEditSize(CEdit& edit, int length) {
  CClientDC dc(edit.m_hWnd);
  const HFONT oldFont = dc.SelectFont(edit.GetFont());
  TEXTMETRIC tm;
  dc.GetTextMetrics(&tm);
  dc.SelectFont(oldFont);
  SIZE result;
  result.cx = (9 * tm.tmAveCharWidth + tm.tmMaxCharWidth) * length / 10 + GetSystemMetrics(SM_CXBORDER) * 8;
  result.cy = tm.tmHeight + GetSystemMetrics(SM_CYBORDER) * 8;
  return result;
}

int ControlAdjuster::getComboBoxHeight(CComboBox &combo) {
  CClientDC dc(combo.m_hWnd);
  const HFONT oldFont = dc.SelectFont(combo.GetFont());
  TEXTMETRIC tm;
  dc.GetTextMetrics(&tm);
  dc.SelectFont(oldFont);
  return tm.tmHeight + GetSystemMetrics(SM_CYBORDER) * 8;
}

SIZE ControlAdjuster::adjustComboBoxSize(CComboBox &combo) {
  SIZE result;
  result.cx = 0;
  CClientDC dc(combo.m_hWnd);
  const HFONT oldFont = dc.SelectFont(combo.GetFont());
  TEXTMETRIC tm;
  dc.GetTextMetrics(&tm);
  result.cy = tm.tmHeight + GetSystemMetrics(SM_CYBORDER) * 8;
  const int itemCount = combo.GetCount();
  int dropDownCount;
  // TODO в будущем передавать параметр для ограничения количества выпадающих элементов
  if (itemCount < 1) {
    dropDownCount = 1;
  }
  else if (itemCount > 15) {
    dropDownCount = 15; 
  }
  else {
    dropDownCount = itemCount;
  }
  result.cy += dropDownCount * combo.GetItemHeight(0) + 2;
  if (itemCount > 0) {    
    AutoArray<int> itemLengths(itemCount);
    int textLen = 0;
    for (int i = 0; i < itemCount; ++i) {
      const int itemLen = combo.GetLBTextLen(i);
      itemLengths[i] = itemLen;
      if (itemLen > textLen) {
        textLen = itemLen;
      }
    }
    if (textLen > 0) {
      AutoArray<TCHAR> text(textLen + 1);
      for (int i = 0; i < itemCount; ++i) {
        combo.GetLBText(i, text);
        SIZE itemSize;
        dc.GetTextExtent(text, itemLengths[i], &itemSize);
        if (itemSize.cx > result.cx) {
          result.cx = itemSize.cx;
        }
      }
    }
  }
  result.cx += 2 * GetSystemMetrics(SM_CXVSCROLL);
  dc.SelectFont(oldFont);
  return result;
}

SIZE ControlAdjuster::adjustStaticSize(CStatic s) {
  SIZE result;
  CClientDC dc(s.m_hWnd);
  const HFONT oldFont = dc.SelectFont(s.GetFont());
  const int len = s.GetWindowTextLength();
  if (len > 0) {
    AutoArray<TCHAR> caption(len + 1);
    s.GetWindowText(caption, len + 1);
    dc.GetTextExtent(caption, len, &result);
  }
  else {
    TEXTMETRIC tm;
    dc.GetTextMetrics(&tm);
    result.cy = tm.tmHeight;
    result.cx = tm.tmAveCharWidth;
  }
  dc.SelectFont(oldFont);
  return result;
}

SIZE ControlAdjuster::adjustStaticSize(CStatic s, bool wordWrap) {
  if (!wordWrap) {
    return adjustStaticSize(s);
  }
  SIZE result;
  CClientDC dc(s.m_hWnd);
  const HFONT oldFont = dc.SelectFont(s.GetFont());
  const int len = s.GetWindowTextLength();
  if (len > 0) {
    AutoArray<TCHAR> caption(len + 1);
    s.GetWindowText(caption, len + 1);
    RECT r;
    s.GetWindowRect(&r);
    dc.DrawText(caption, len, &r, DT_CALCRECT | DT_WORDBREAK | DT_LEFT);
    result.cx = r.right - r.left;
    result.cy = r.bottom - r.top;
  }
  else {
    TEXTMETRIC tm;
    dc.GetTextMetrics(&tm);
    result.cy = tm.tmHeight;
    result.cx = tm.tmAveCharWidth;
  }
  dc.SelectFont(oldFont);
  return result;
}

int ControlAdjuster::adjustHeaderHeight(HWND hwnd) {
  CClientDC dc(hwnd);
  const CaptionFont font(CaptionFont::BOLD);
  const HFONT oldFont = dc.SelectFont(font);
  TEXTMETRIC tm;
  dc.GetTextMetrics(&tm);
  dc.SelectFont(oldFont);
  return max((int) HEADER_ICON_HEIGHT, (int) tm.tmHeight * 2);
}

POINT ControlAdjuster::getWindowOrigin(HWND hwnd) {
  CWindow wnd(hwnd);
  CRect r;
  wnd.GetWindowRect(&r);
  CWindow(wnd.GetParent()).ScreenToClient(&r);
  return r.TopLeft();
}

#if (_WIN32_WINNT >= 0x0501)
bool ControlAdjuster::isWindows2000() {
  const WORD version = LOWORD(GetVersion());
  return LOBYTE(version) == 5 && HIBYTE(version) == 0;
}
#endif

/*
void ControlAdjuster::resizeWindow(HWND hwnd, const SIZE& windowSize) {
  CWindow wnd(hwnd);
  RECT r;
  wnd.GetWindowRect(&r);
  CWindow(wnd.GetParent()).ScreenToClient(&r);
  wnd.MoveWindow(r.left, r.top, windowSize.cx, windowSize.cy);
}

void ControlAdjuster::moveWindow(HWND hwnd, int x, int y) {
  CWindow wnd(hwnd);
  CRect r;
  wnd.GetWindowRect(&r);
  CWindow(wnd.GetParent()).ScreenToClient(&r);
  wnd.MoveWindow(x, y, r.Width(), r.Height());
}
*/
