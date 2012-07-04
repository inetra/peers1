#include "stdafx.h"
#include "HelpHyperLink.h"
#include "CaptionFont.h"

enum {
  LEFT_PADDING = 8,
  LEFT_MARGIN = 8,
  RIGHT_MARGIN = 2,
  TOP_MARGIN = 1,
  BOTTOM_MARGIN = 1,
  HELP_ICON_WIDTH = 16,
  HELP_ICON_HEIGHT = 16
};

void CHelpHyperLink::Init() {
  CHyperLinkImpl::Init();
  helpIcon.LoadIcon(IDI_SETTINGS_HELP);
  CaptionFont font(GetFont(), 7, 8);
  smallFont.Attach(font.Detach());
  SetFont(smallFont);
  SetLinkFont(smallFont);
}

void CHelpHyperLink::DoEraseBackground(CDCHandle dc) {
  RECT r;
  GetClientRect(&r);
  const int radius = (r.bottom - r.top) / 3;
  {
    CRect tmp(r.left, r.top, r.left + radius, r.bottom);
    dc.FillRect(tmp, GetSysColorBrush(COLOR_BTNFACE));
  }
  HBRUSH brush = GetSysColorBrush(COLOR_INFOBK);    
  CPen pen;
  pen.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_INFOBK));
  const HBRUSH oldBrush = dc.SelectBrush(brush);
  const HPEN oldPen = dc.SelectPen(pen);
  dc.Pie(r.left, r.top, r.left + 2*radius, r.top + 2*radius, r.left + radius, r.top, r.left, r.top + radius);
  dc.Pie(r.left, r.bottom - 2*radius, r.left + 2*radius, r.bottom, r.left, r.bottom - radius, r.left + radius, r.bottom);
  if (2 * radius < r.bottom) {
    CRect tmp(r.left, r.top + radius, r.left + radius, r.bottom - radius);
    dc.FillRect(tmp, brush);
  }
  r.left += radius;
  dc.FillRect(&r, brush);
  dc.SelectBrush(oldBrush);
  dc.SelectPen(oldPen);
}

void CHelpHyperLink::DoPaint(CDCHandle dc) {
  dc.SetBkMode(TRANSPARENT);
  const COLORREF clrOld = dc.SetTextColor(IsWindowEnabled() ? m_clrLink : (::GetSysColor(COLOR_GRAYTEXT)));
  HFONT hFontOld;
  if (m_hFont != NULL && (!IsUnderlineHover() || (IsUnderlineHover() && m_bHover))) {
    hFontOld = dc.SelectFont(m_hFont);
  }
  else {
    hFontOld = dc.SelectFont(m_hFontNormal);
  }
  const LPTSTR lpstrText = (m_lpstrLabel != NULL) ? m_lpstrLabel : m_lpstrHyperLink;
  dc.DrawText(lpstrText, -1, &m_rcLink, DT_LEFT | DT_WORDBREAK);
  dc.SetTextColor(clrOld);
  dc.SelectFont(hFontOld);
  helpIcon.DrawIconEx(dc,
    m_rcLink.left - HELP_ICON_WIDTH - LEFT_PADDING,
    max(m_rcLink.top, (m_rcLink.top + m_rcLink.bottom - HELP_ICON_HEIGHT) / 2),
    HELP_ICON_WIDTH,
    HELP_ICON_HEIGHT);
}

bool CHelpHyperLink::CalcLabelRect() {
  bool result = CHyperLinkImpl::CalcLabelRect();
  if (result) {
    RECT rcClient;
    GetClientRect(&rcClient);
    int dx = rcClient.right - m_rcLink.right - RIGHT_MARGIN;
    ::OffsetRect(&m_rcLink, dx, 0);
    rcClient.top += TOP_MARGIN;
    rcClient.bottom += TOP_MARGIN + BOTTOM_MARGIN;
  }
  return result;
}

bool CHelpHyperLink::GetIdealSize(int& cx, int& cy, int clientWidth) const {
  ATLASSERT(::IsWindow(m_hWnd));
  if (m_lpstrLabel == NULL && m_lpstrHyperLink == NULL) {
    return false;
  }
  if (!m_bPaintLabel) {
    return false;
  }
  CClientDC dc(m_hWnd);
  RECT rcClient = { 0, 0, clientWidth, 0 };
  RECT rcAll = rcClient;
  HFONT hOldFont = NULL;
  if (m_hFont != NULL) {
    hOldFont = dc.SelectFont(m_hFont);
  }
  LPTSTR lpstrText = (m_lpstrLabel != NULL) ? m_lpstrLabel : m_lpstrHyperLink;
  DWORD dwStyle = GetStyle();
  int nDrawStyle = DT_LEFT;
  if (dwStyle & SS_CENTER) {
    nDrawStyle = DT_CENTER;
  }
  else if (dwStyle & SS_RIGHT) {
    nDrawStyle = DT_RIGHT;
  }
  dc.DrawText(lpstrText, -1, &rcAll, nDrawStyle | DT_WORDBREAK | DT_CALCRECT);
  if (m_hFont != NULL) {
    dc.SelectFont(hOldFont);
  }
  if (dwStyle & SS_CENTER) {
    int dx = (rcClient.right - rcAll.right) / 2;
    ::OffsetRect(&rcAll, dx, 0);
  }
  else if (dwStyle & SS_RIGHT) {
    int dx = rcClient.right - rcAll.right;
    ::OffsetRect(&rcAll, dx, 0);
  }
  cx = rcAll.right - rcAll.left;
  cy = rcAll.bottom - rcAll.top;
  cx += LEFT_MARGIN + LEFT_PADDING + RIGHT_MARGIN + HELP_ICON_WIDTH;
  cy = max(cy, (int) HELP_ICON_HEIGHT) + TOP_MARGIN + BOTTOM_MARGIN;
  return true;
}
