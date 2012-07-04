#include "stdafx.h"
#include "EditIcon.h"

HWND CEditIcon::Create(HWND hWndParent, ATL::_U_STRINGorID resourceId) {
  m_bitmap.init(resourceId);
  CRect r(0, 0, m_bitmap.getWidth(), m_bitmap.getHeight());
  return CWindowImpl<CEditIcon>::Create(hWndParent, r, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
}

LRESULT CEditIcon::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CPaintDC dc(m_hWnd);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_WINDOW));
  m_bitmap.draw(dc, 0, 0);
  return 0;
}
