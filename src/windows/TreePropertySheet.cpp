/* 
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"

#include "TreePropertySheet.h"

#define NO_PROPERTY_SHEET -1

static const TCHAR SEPARATOR = _T('\\');

void TreePropertySheet::OnSheetInitialized() {
  PostMessage(WM_USER_INITDIALOG);
}

LRESULT TreePropertySheet::onUserInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (ResourceManager::getInstance()->isRTL()) {
    SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) | WS_EX_LAYOUTRTL);
  }
  tree_icons.CreateFromImage(IDB_O_SETTINGS_DLG, 16, 23, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
  hideTab();
  const int pageCount = CTabCtrl(GetTabControl()).GetItemCount();
  if (pageCount > 1) {
    addTree();
    fillTree();
  }
  CenterWindow();
  return 0;
}

void TreePropertySheet::hideTab() {
	CRect rcClient, rcTab, rcPage, rcWindow;
	CWindow tab = GetTabControl();
	CWindow page = IndexToHwnd(0);

	GetClientRect(&rcClient);

	tab.GetWindowRect(&rcTab);
	page.GetClientRect(&rcPage);
	page.MapWindowPoints(m_hWnd,&rcPage);
	GetWindowRect(&rcWindow);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcTab, 2);

        const int pageCount = CTabCtrl(GetTabControl()).GetItemCount();
        const int deltaX = pageCount > 1 ? SPACE_LEFT + TREE_WIDTH + SPACE_MID - rcPage.left : SPACE_LEFT - rcPage.left;

        ScrollWindow(deltaX, SPACE_TOP-rcPage.top);
        rcWindow.right += deltaX - (rcClient.Width() - rcTab.right) + SPACE_RIGHT;
        rcWindow.bottom += SPACE_TOP - rcPage.top;

	tab.ShowWindow(SW_HIDE);

	MoveWindow(&rcWindow, TRUE);

	tabContainer.SubclassWindow(tab.m_hWnd);
}

void TreePropertySheet::addTree()
{
	// Insert the space to the left
	CRect rcPage;

	HWND page = IndexToHwnd(0);
	::GetWindowRect(page, &rcPage);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcPage, 2);

	CRect rc(SPACE_LEFT, rcPage.top, TREE_WIDTH, rcPage.bottom);
	ctrlTree.Create(m_hWnd, rc, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_HASBUTTONS | /*TVS_HASLINES |*/ TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP, WS_EX_CLIENTEDGE, IDC_PAGE);
	ctrlTree.SetImageList(tree_icons, TVSIL_NORMAL);
}

void TreePropertySheet::fillTree() {
  CTabCtrl tab = GetTabControl();
  int pages = tab.GetItemCount();

  TCHAR buf[MAX_NAME_LENGTH];
  TCITEM item;
  item.mask = TCIF_TEXT;
  item.pszText = buf;
  item.cchTextMax = MAX_NAME_LENGTH - 1;

  for (int i = 0; i < pages; ++i) {
    if (isVisiblePage(i)) {
      tab.GetItem(i, &item);
      createTree(buf, TVI_ROOT, i);
    }
  }
}

HTREEITEM TreePropertySheet::createTree(const tstring& str, HTREEITEM parent, int page) {
	TVINSERTSTRUCT tvi;
	tvi.hInsertAfter = TVI_LAST;
	tvi.hParent = parent;

	HTREEITEM first = (parent == TVI_ROOT) ? ctrlTree.GetRootItem() : ctrlTree.GetChildItem(parent);

	string::size_type i = str.find(SEPARATOR);
	if(i == string::npos) {
		// Last dir, the actual page
		HTREEITEM item = findItem(str, first);
		if(item == NULL) {
			// Doesn't exist, add
			tvi.item.mask = TVIF_PARAM | TVIF_TEXT;
			tvi.item.pszText = const_cast<LPTSTR>(str.c_str());
			tvi.item.lParam = page;
			item = ctrlTree.InsertItem(&tvi);
                        const int imageIndex = getItemImageIndex(page);
			ctrlTree.SetItemImage(item, imageIndex, imageIndex);
			ctrlTree.Expand(parent);
			return item;
		} else {
			// Update page
			if(ctrlTree.GetItemData(item) == NO_PROPERTY_SHEET)
				ctrlTree.SetItemData(item, page);
			return item;
		}
	} else {
		tstring name = str.substr(0, i);
		HTREEITEM item = findItem(name, first);
		if(item == NULL) {
			// Doesn't exist, add...
			tvi.item.mask = TVIF_PARAM | TVIF_TEXT;
			tvi.item.lParam = NO_PROPERTY_SHEET;
			tvi.item.pszText = const_cast<LPTSTR>(name.c_str());
			item = ctrlTree.InsertItem(&tvi);
                        const int imageIndex = getItemImageIndex(page);
			ctrlTree.SetItemImage(item, imageIndex, imageIndex);
		} 
		ctrlTree.Expand(parent);
		// Recurse...
		return createTree(str.substr(i+1), item, page);
	}	
}

HTREEITEM TreePropertySheet::findItem(const tstring& str, HTREEITEM start) {
	TCHAR buf[MAX_NAME_LENGTH];

	while(start != NULL) {
		ctrlTree.GetItemText(start, buf, MAX_NAME_LENGTH-1);
		if(str == buf) {
			return start;
		}
		start = ctrlTree.GetNextSiblingItem(start);
	}
	return start;
}

HTREEITEM TreePropertySheet::findItem(int page, HTREEITEM start) {
  while (start != NULL) {
    const int nodeData = (int) ctrlTree.GetItemData(start);
    if (nodeData == page) {
      return start;
    }
    const HTREEITEM ret = findItem(page, ctrlTree.GetChildItem(start));
    if (ret != NULL) {
      return ret;
    }
    start = ctrlTree.GetNextSiblingItem(start);
  }
  return NULL;
}

LRESULT TreePropertySheet::onSelChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /* bHandled */) {
  NMTREEVIEW* nmtv = (NMTREEVIEW*)pnmh;
  if (allowChange(nmtv)) {
    int page = nmtv->itemNew.lParam;
    if (page == NO_PROPERTY_SHEET) {
      HTREEITEM next = ctrlTree.GetChildItem(nmtv->itemNew.hItem);
      if (next == NULL) {
        next = ctrlTree.GetNextSiblingItem(nmtv->itemNew.hItem);
        if (next == NULL) {
          next = ctrlTree.GetParentItem(nmtv->itemNew.hItem);
          if (next != NULL) {
            next = ctrlTree.GetNextSiblingItem(next);
          }
        }
      }
      if (next != NULL) {
        ctrlTree.SelectItem(next);
        onPageChanged((int) ctrlTree.GetItemData(next));
      }
    } 
    else if (HwndToIndex(GetActivePage()) != page) {
      SetActivePage(page);
      onPageChanged(page);
    }
  }
  return 0;
}

void TreePropertySheet::onPageChanged(int /*pageIndex*/) {
}

LRESULT TreePropertySheet::onSetCurSel(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	ctrlTree.SelectItem(findItem((int)wParam, ctrlTree.GetRootItem()));
	bHandled = FALSE;
	return 0;
}

/**
* @file
* $Id: TreePropertySheet.cpp,v 1.11 2008/03/10 07:51:06 alexey Exp $
*/
