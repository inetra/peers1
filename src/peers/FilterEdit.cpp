#include "stdafx.h"
#include "FilterEdit.h"

#define CLEAR_BUTTON_WIDTH 12
#define CLEAR_BUTTON_HEIGHT 12
#define CLEAR_BUTTON_RIGHT_MARGIN 1

void CFilterEdit::onSize() {
  if (clearButtonVisible) {
    CRect r;
    GetClientRect(r);
    updateClearButtonRect(r);
    clearButton.MoveWindow(r);
  }
  CEditWithIcon::onSize();
}

void CFilterEdit::updateEditRect(CRect& r) {
  CEditWithIcon::updateEditRect(r);
  if (clearButtonVisible) {
    r.right -= CLEAR_BUTTON_WIDTH - CLEAR_BUTTON_RIGHT_MARGIN;
  }
}

void CFilterEdit::updateClearButtonRect(CRect &r) {
  r.top = (r.top + r.bottom - CLEAR_BUTTON_HEIGHT) / 2;
  r.bottom = r.top + CLEAR_BUTTON_HEIGHT;
  r.right -= CLEAR_BUTTON_RIGHT_MARGIN;
  r.left = r.right - CLEAR_BUTTON_WIDTH;
}

void CFilterEdit::showClearButton() {
  if (clearButtonVisible) return;
  CRect r;
  GetClientRect(r);
  updateClearButtonRect(r);
  clearButton.Create(m_hWnd, r, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW, 0, IDC_USER_LIST_FILTER_CLEAR);
  clearButtonVisible = true;
  setEditRect();
}

void CFilterEdit::hideClearButton() {
  if (!clearButtonVisible) return;
  clearButton.DestroyWindow();
  clearButton.Detach();
  clearButtonVisible = false;
  setEditRect();
}

LRESULT CFilterEdit::onClearButtonClick(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
  SetWindowText(_T(""));
  ::SendMessage(GetParent(), WM_COMMAND, MAKEWPARAM(GetWindowLong(GWL_ID), EN_CHANGE), (LPARAM) m_hWnd);
  return 0;
}

LRESULT CClearButton::onDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT) lParam;
  dcassert(lpDrawItem->CtlType == ODT_BUTTON);
  CDCHandle dc(lpDrawItem->hDC);
  CRect r;
  GetClientRect(r);
  dc.FillRect(r, GetSysColorBrush(COLOR_WINDOW));
  CBitmap bmp;
  bmp.LoadBitmap(IDB_USER_LIST_FILTER_CLEAR);
  CDC comp;
  comp.CreateCompatibleDC(dc);    
  comp.SelectBitmap(bmp);
  int x = 1, y = 1;
  if (lpDrawItem->itemState & ODS_SELECTED) {
    ++x;
    ++y;
  }
  // размер картинки 10, а кнопки 12
  dc.BitBlt(x, y, CLEAR_BUTTON_WIDTH - 2, CLEAR_BUTTON_HEIGHT - 2, comp, 0, 0, SRCCOPY);
  return TRUE;
}
