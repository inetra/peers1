#include "stdafx.h"
#include "PeersToolbar.h"
#include "../client/SearchManager.h"

#define IMAGELIST_WIDTH 16
#define IMAGELIST_HEIGHT 16

/* отступ от края до картинки */
#define ITEM_LEFT_MARGIN 2
/* отступ между картинкой и текстом */
#define ITEM_LEFT_PADDING 4
#define ITEM_TOP_MARGIN 1
#define ITEM_BOTTOM_MARGIN 1

ResourceManager::Strings CFileTypeComboBox::searchTypeNames[] = {
  ResourceManager::ANY,
  ResourceManager::AUDIO,
  ResourceManager::COMPRESSED,
  ResourceManager::DOCUMENT,
  ResourceManager::EXECUTABLE,
  ResourceManager::PICTURE,
  ResourceManager::VIDEO,
  ResourceManager::DIRECTORY,
  ResourceManager::SEARCH_TYPE_TTH,
  ResourceManager::SEARCH_TYPE_CD_IMAGE
};

LRESULT CFileTypeComboBox::onCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
  DefWindowProc(uMsg, wParam, lParam);
  searchTypes.CreateFromImage(IDB_SEARCH_TYPES, IMAGELIST_WIDTH, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
  SetFont(WinUtil::systemFont, FALSE);
  const int itemHeight = calculateItemHeight();
  SetItemHeight(-1, itemHeight);
  SetItemHeight(0, itemHeight);
  dcassert(COUNTOF(searchTypeNames) == SearchManager::TYPE_CD_IMAGE + 1);
  for (int i = 0; i < COUNTOF(searchTypeNames); ++i) {
    AddString(CTSTRING_I(searchTypeNames[i]));
  }
  SetCurSel(0);
  return 0;
}

LRESULT CFileTypeComboBox::onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  searchTypes.Destroy();
  bHandled = FALSE;
  return 0;
}

LRESULT CFileTypeComboBox::onDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
  TCHAR szText[MAX_PATH+1];
  switch (dis->itemAction) 
  {
  case ODA_FOCUS:
    if (!(dis->itemState & ODS_NOFOCUSRECT)) {
      DrawFocusRect(dis->hDC, &dis->rcItem);
    }
    break;
  case ODA_SELECT:
  case ODA_DRAWENTIRE:
    GetLBText(dis->itemID, szText);
    if (dis->itemState & ODS_SELECTED) {
      SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
      SetBkColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHT));
    } 
    else {			
      SetTextColor(dis->hDC, WinUtil::textColor);
      SetBkColor(dis->hDC, WinUtil::bgColor);
    }
    const HFONT oldFont = (HFONT) SelectObject(dis->hDC, GetFont());
    CDCHandle dc(dis->hDC);
    SIZE textSize;
    dc.GetTextExtent(szText, lstrlen(szText), &textSize);
    ExtTextOut(dis->hDC, dis->rcItem.left + ITEM_LEFT_MARGIN + IMAGELIST_WIDTH + ITEM_LEFT_PADDING, (dis->rcItem.top + dis->rcItem.bottom - textSize.cy) / 2, ETO_OPAQUE, &dis->rcItem, szText, lstrlen(szText), 0);
    SelectObject(dis->hDC, oldFont);
    if (dis->itemState & ODS_FOCUS) {
      if (!(dis->itemState & ODS_NOFOCUSRECT)) {
        DrawFocusRect(dis->hDC, &dis->rcItem);
      }
    }
    ImageList_Draw(searchTypes, dis->itemID, dis->hDC, dis->rcItem.left + ITEM_LEFT_MARGIN, (dis->rcItem.top + dis->rcItem.bottom - IMAGELIST_HEIGHT) / 2, ILD_TRANSPARENT);
    break;
  }
  return TRUE;
}

LRESULT CFileTypeComboBox::onMeasure(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  LPMEASUREITEMSTRUCT mis = (LPMEASUREITEMSTRUCT) lParam;
  dcassert(mis->CtlType == ODT_COMBOBOX);
  mis->itemHeight = calculateItemHeight();
  return TRUE;
}

int CFileTypeComboBox::calculateItemHeight() const {
  CClientDC dc(m_hWnd);
  const HFONT oldFont = dc.SelectFont(GetFont());
  TEXTMETRIC tm;
  dc.GetTextMetrics(&tm);
  dc.SelectFont(oldFont);
  return max((int)tm.tmHeight, IMAGELIST_HEIGHT) + ITEM_TOP_MARGIN + ITEM_BOTTOM_MARGIN;
}

int CFileTypeComboBox::getExtraItemWidth() const {
  return IMAGELIST_WIDTH + ITEM_LEFT_MARGIN + ITEM_LEFT_PADDING;
}
