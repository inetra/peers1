#include "stdafx.h"
#include "EditWithIcon.h"

LRESULT CEditWithIcon::onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
  DefWindowProc(uMsg, wParam, lParam);
  icon.Create(m_hWnd, m_resourceId);
  setEditRect();
  return 0;
}

LRESULT CEditWithIcon::onSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
  DefWindowProc(uMsg, wParam, lParam);
  onSize();
  return 0;
}

void CEditWithIcon::onSize() {
  setEditRect();
}

void CEditWithIcon::setEditRect() {
	CRect r;
	GetClientRect(r);
	CRect savedR(r);
	updateEditRect(r);
#ifdef _DEBUG
	dcassert(GetStyle() & ES_MULTILINE);
#endif
	if (r != savedR) {
		SetRectNP(r);
	}
}

void CEditWithIcon::updateEditRect(CRect& r) {
  if (icon) {
#if 0
    const int top = (r.Height() - icon.getHeight()) / 2;
    const CRect iconRect(r.left, top, r.left + icon.getWidth(), top + icon.getHeight());
    icon.MoveWindow(iconRect);
#endif
    r.left += icon.getWidth();
  }
}

tstring CEditWithIcon::getText() const {
  const int filterTextLength = GetWindowTextLength();
  AutoArray<TCHAR> buf(filterTextLength + 1);
  GetWindowText(buf, filterTextLength + 1);
  return (TCHAR*) buf;
}
