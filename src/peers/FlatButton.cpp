#include "stdafx.h"
#include "FlatButton.h"
#include "ControlAdjuster.h"

FlatButton::FlatButton(): m_alignment(taLeft) { 
  init();
}

FlatButton::FlatButton(TextAlignment alignment): m_alignment(alignment) { 
  init();
}

void FlatButton::init() {
  m_hintTime = HOVER_DEFAULT;
  m_enabled = true;
  m_allowFocus = true;
  m_hover = false;
  m_leftPadding = 2;
  m_highLightBorder = false;
  m_textColor = GetSysColor(COLOR_BTNTEXT);
  m_backgroundColor = GetSysColor(COLOR_BTNFACE);
  m_hiliteColor = GetSysColor(COLOR_BTNHILIGHT);
  m_highLightBorderColor = GetSysColor(COLOR_BTNSHADOW);
}

HWND FlatButton::Create(HWND hWndParent, LPCTSTR szWindowName, _U_MENUorID MenuOrID) {
  return CWindowImpl<FlatButton, CButton>::Create(hWndParent, rcDefault, szWindowName, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_OWNERDRAW, 0, MenuOrID);
}

HWND FlatButton::Create(HWND hWndParent, ATL::_U_RECT rect, LPCTSTR szWindowName, _U_MENUorID MenuOrID) {
  return CWindowImpl<FlatButton, CButton>::Create(hWndParent, rect, szWindowName, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | BS_OWNERDRAW, 0, MenuOrID);
}

LRESULT FlatButton::onDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  //dcdebug("FlatButton::onDrawItem %x\n", m_hWnd);
  LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
  dcassert(lpDrawItem->CtlType == ODT_BUTTON);
  CDCHandle dc(lpDrawItem->hDC);
  CRect r;
  GetClientRect(r);
  drawBackrgound(dc, r);
  if (m_alignment == taLeft) {        
    r.left += m_leftPadding;
    if (m_hover && m_highLightBorder) {
      r.left -= 1;
    }
  }
  drawButton(dc, r, lpDrawItem->itemState & ODS_SELECTED);
  return TRUE;
}

void FlatButton::drawBackrgound(CDCHandle dc, CRect &r) {
	if (m_hover && m_highLightBorder) {
		CPen pen;
		pen.CreatePen(PS_SOLID, 1, m_highLightBorderColor);
		const HPEN oldPen = dc.SelectPen(pen);
		dc.MoveTo(r.left, r.top);
		dc.LineTo(r.right - 1, r.top);
		dc.LineTo(r.right - 1, r.bottom - 1);
		dc.LineTo(r.left, r.bottom - 1);
		dc.LineTo(r.left, r.top);
		dc.SelectPen(oldPen);
		r.InflateRect(-1, -1);
	}
	CBrush brush;
	brush.CreateSolidBrush(m_hover ? m_hiliteColor : m_backgroundColor);
	dc.FillRect(r, brush);
}

void FlatButton::drawButton(CDCHandle dc, const CRect &r, bool selected) {
  //dcdebug("FlatButton::drawButton %x\n", m_hWnd);
  const int len = GetWindowTextLength();
  if (len > 0) {
    AutoArray<TCHAR> caption(len + 1);
    GetWindowText(caption, len + 1);
    CRect textRect(r);
    if (selected) {
      textRect.OffsetRect(1, 1);
    }
    dc.SetBkMode(TRANSPARENT);
    dc.SetTextColor(m_enabled ? m_textColor : GetSysColor(COLOR_GRAYTEXT));
    UINT drawTextFormat = DT_SINGLELINE | DT_VCENTER;
    switch (m_alignment) {
      case taLeft:
        drawTextFormat |= DT_LEFT;
        break;
      case taRight:
        drawTextFormat |= DT_RIGHT;
        break;
      case taCenter:
        drawTextFormat |= DT_CENTER;
        break;
    }
    dc.DrawText(caption, len, textRect, drawTextFormat);
  }
}

LRESULT FlatButton::onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  if (!m_hover && m_enabled) {
    TRACKMOUSEEVENT tme;
    ZeroMemory(&tme, sizeof(TRACKMOUSEEVENT));
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE;
    if (m_hintTime != 0 && !m_hintText.empty()) {
      tme.dwFlags |= TME_HOVER;
      tme.dwHoverTime = m_hintTime;
    }
    tme.hwndTrack = m_hWnd;
    TrackMouseEvent(&tme);
    Invalidate();
    m_hover = true;
  }
  bHandled = FALSE;
  return 0;
}

LRESULT FlatButton::onMouseLeave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  m_hover = false;
  if (m_hintControl) {
    m_hintControl.DestroyWindow();
    m_hintControl.Detach();
  }
  Invalidate();
  return 0;
}

LRESULT FlatButton::onMouseHover(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (!m_hintText.empty()) {
    if (!m_hintControl) {
      m_hintControl.Create(m_hWnd, rcDefault, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP);    
      CRect r;
      GetClientRect(r);
      CToolInfo ti(0, m_hWnd, (UINT_PTR) 1, r, const_cast<LPTSTR>(m_hintText.c_str()));
      ControlAdjuster::fixToolInfoSize(ti);
      m_hintControl.AddTool(&ti);
    }
    CToolInfo ti(0, m_hWnd, (UINT_PTR) 1, NULL, NULL);
    ControlAdjuster::fixToolInfoSize(ti);
    m_hintControl.TrackActivate(ti,TRUE);
  }
  return 0;
}

SIZE FlatButton::getPreferredSize() {
  SIZE result= { 0 };
  CClientDC dc(m_hWnd);
  const HFONT oldFont = dc.SelectFont(GetFont());
  const int len = GetWindowTextLength();
  if (len > 0) {
    AutoArray<TCHAR> caption(len + 1);
    GetWindowText(caption, len + 1);
    dc.GetTextExtent(caption, len, &result);
  }
  if (m_highLightBorder) {
    result.cx += 2;
  }
  if (m_alignment == taLeft) {
    result.cx += m_leftPadding;
  }
  result.cx += 4;
  dc.SelectFont(oldFont);
  return result;
}

LRESULT FlatButton::onEnable(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
  m_enabled = (wParam != 0);
  bHandled = FALSE;
  return 0;
}

LRESULT FlatButton::onLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  if (!isAllowFocus()) {
    SetCapture();
    SetState(TRUE);
  }
  else {
    bHandled = FALSE;
  }
  return 0;
}

LRESULT FlatButton::onLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
  if (!isAllowFocus()) {
    ReleaseCapture();
    SetState(FALSE);
    const int xPos = GET_X_LPARAM(lParam), yPos = GET_Y_LPARAM(lParam);
    CRect r;
    GetClientRect(r);
    if (xPos >= r.left && xPos < r.right && yPos >= r.top && yPos < r.bottom) {
      ::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(GetWindowLong(GWL_ID), BN_CLICKED), 0);
    }
  }
  else {
    bHandled = FALSE;
  }
  return 0;
}
