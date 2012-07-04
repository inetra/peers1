#pragma once
#include "TransparentBitmap.h"

class CEditIcon : public CWindowImpl<CEditIcon> {
  BEGIN_MSG_MAP(CEditIcon)
    MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkGnd);
    MESSAGE_HANDLER(WM_PAINT, onPaint);
  END_MSG_MAP()

  LRESULT onEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
  LRESULT onPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
public:
  HWND Create(HWND hWndParent, ATL::_U_STRINGorID resourceId);
  int getWidth() const { return m_bitmap.getWidth(); }
  int getHeight() const { return m_bitmap.getHeight(); }
private:
  TransparentBitmap m_bitmap;
};
