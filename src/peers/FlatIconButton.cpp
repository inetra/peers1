#include "stdafx.h"
#include "FlatIconButton.h"

#define ICON_WIDTH  32
#define ICON_HEIGHT 32
#define ICON_GAP     2

HWND FlatIconButton::Create(ATL::_U_STRINGorID resourceId, HWND hWndParent, LPCTSTR szWindowName, _U_MENUorID MenuOrID) {
  if (resourceId.m_lpstr != NULL) {
    m_icon.LoadIcon(resourceId);
  }
  return FlatButton::Create(hWndParent, szWindowName, MenuOrID);
}

void FlatIconButton::drawDisabledIcon(CDCHandle dc, int x, int y) {
  CBitmap original;
  original.CreateCompatibleBitmap(dc, ICON_WIDTH, ICON_HEIGHT);
  CDC originalDC;
  originalDC.CreateCompatibleDC(dc);
  const HBITMAP oldBitmap = originalDC.SelectBitmap(original);
  CRect targetRect(0, 0, ICON_WIDTH, ICON_HEIGHT);
  originalDC.FillRect(targetRect, (HBRUSH) GetStockObject(WHITE_BRUSH));
  getCurrentIcon().DrawIcon(originalDC, 0, 0);
  dc.DitherBlt(x, y, ICON_WIDTH, ICON_HEIGHT, originalDC, NULL, 0, 0);
  originalDC.SelectBitmap(oldBitmap);
}

void FlatIconButton::drawButton(CDCHandle dc, const CRect &r, bool selected) {
  CIcon& icon = getCurrentIcon();
  if (icon) {
    int x = r.left;
    int y = r.top;
    if (IsWindowEnabled()) {
      if (selected) {
        ++x;
        ++y;
      }
      icon.DrawIcon(dc, x, y + (r.Height() - ICON_HEIGHT) / 2);
    }
    else {
      dcassert(!selected);
      drawDisabledIcon(dc, x, y + (r.Height() - ICON_HEIGHT) / 2);
    }
    CRect buttonRect(r);
    buttonRect.left += ICON_WIDTH + ICON_GAP;
    FlatButton::drawButton(dc, buttonRect, selected);
  }
  else {
    FlatButton::drawButton(dc, r, selected);
  }
}

SIZE FlatIconButton::getPreferredSize() {
  SIZE result = FlatButton::getPreferredSize();
  result.cx += ICON_WIDTH + ICON_GAP;
  result.cy = max((int)result.cy, ICON_HEIGHT);
  return result;
}
