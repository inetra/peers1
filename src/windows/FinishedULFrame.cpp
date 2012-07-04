/* 
 * Copyright (C) 2001-2005 Jacek Sieka, arnetheduck on gmail point com
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

#include "FinishedULFrame.h"
#include "CZDCLib.h"	
#include "TextFrame.h"

int FinishedULFrame::columnIndexes[] = { COLUMN_DONE, COLUMN_FILE, COLUMN_PATH, COLUMN_NICK, COLUMN_HUB, COLUMN_SIZE, COLUMN_SPEED };
int FinishedULFrame::columnSizes[] = { 100, 110, 290, 125, 80, 80, 80 };
static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::TIME, ResourceManager::PATH, 
ResourceManager::NICK, ResourceManager::HUB, ResourceManager::SIZE, ResourceManager::SPEED
};

LRESULT FinishedULFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_FINISHED_UL);
	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);	

	ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(FINISHED_UL_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(FINISHED_UL_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlList.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlList.setSort(COLUMN_DONE, ExListViewCtrl::SORT_STRING_NOCASE);
	
	UpdateLayout();
	
	FinishedManager::getInstance()->addListener(this);
	updateList(FinishedManager::getInstance()->lockList(true));
	FinishedManager::getInstance()->unlockList();
	
	ctxMenu.CreatePopupMenu();
	ctxMenu.AppendMenu(MF_STRING, IDC_VIEW_AS_TEXT, CTSTRING(VIEW_AS_TEXT));
	ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FILE, CTSTRING(OPEN));
	ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CTSTRING(OPEN_FOLDER));
	ctxMenu.AppendMenu(MF_SEPARATOR);
	ctxMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));
	ctxMenu.AppendMenu(MF_STRING, IDC_TOTAL, CTSTRING(REMOVE_ALL));
	ctxMenu.SetMenuDefaultItem(IDC_OPEN_FILE);

	bHandled = FALSE;
	return TRUE;
}

LRESULT FinishedULFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() > 0) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		
		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlList, pt);
		}

		ctxMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);			
		return TRUE; 
	}

	bHandled = FALSE;
	return FALSE; 
}

LRESULT FinishedULFrame::onColumnClickFinished(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMLISTVIEW* const l = (NMLISTVIEW*)pnmh;
	if(l->iSubItem == ctrlList.getSortColumn()) {
		if (!ctrlList.isAscending())
			ctrlList.setSort(-1, ctrlList.getSortType());
		else
			ctrlList.setSortDirection(false);
	} else {
		switch(l->iSubItem) {
			case COLUMN_SIZE:
				ctrlList.setSort(l->iSubItem, ExListViewCtrl::SORT_FUNC, true, sortSize);
				break;
			case COLUMN_SPEED:
				ctrlList.setSort(l->iSubItem, ExListViewCtrl::SORT_FUNC, true, sortSpeed);
				break;
			default:
				ctrlList.setSort(l->iSubItem, ExListViewCtrl::SORT_STRING_NOCASE);
				break;
		}
	}
	return 0;
}

void FinishedULFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);

	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[4];
		ctrlStatus.GetClientRect(sr);
		w[3] = sr.right - 16;
		w[2] = max(w[3] - 100, 0);
		w[1] = max(w[2] - 100, 0);
		w[0] = max(w[1] - 100, 0);

		ctrlStatus.SetParts(4, w);
	}

	CRect rc(rect);
	ctrlList.MoveWindow(rc);
}

LRESULT FinishedULFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	
	NMITEMACTIVATE * const item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
		FinishedItem* entry = (FinishedItem*)ctrlList.GetItemData(item->iItem);
		WinUtil::openFile(Text::toT(entry->getTarget()));
	}
	return 0;
}

LRESULT FinishedULFrame::onViewAsText(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		TextFrame::openWindow(Text::toT(entry->getTarget()));
	}
	return 0;
}

LRESULT FinishedULFrame::onOpenFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		WinUtil::openFile(Text::toT(entry->getTarget()));
	}
	return 0;
}

LRESULT FinishedULFrame::onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		WinUtil::openFolder(Text::toT(entry->getTarget()));
	}
	return 0;
}

LRESULT FinishedULFrame::onRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch(wID)
	{
	case IDC_REMOVE:
		{
			int i = -1;
			while((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
				FinishedManager::getInstance()->remove((FinishedItem*)ctrlList.GetItemData(i), true);
				ctrlList.DeleteItem(i);
			}
			break;
		}
	case IDC_TOTAL:
		FinishedManager::getInstance()->removeAll(true);
		break;
	}
	return 0;
}

LRESULT FinishedULFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FinishedManager::getInstance()->removeListener(this);

		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		WinUtil::saveHeaderOrder(ctrlList, SettingsManager::FINISHED_UL_ORDER, 
			SettingsManager::FINISHED_UL_WIDTHS, COLUMN_LAST, columnIndexes, columnSizes);
		CZDCLib::setButtonPressed(IDC_FINISHED_UL, false);

		bHandled = FALSE;
	return 0;
	}
}

LRESULT FinishedULFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == SPEAK_ADD_LINE) {
		FinishedItem* entry = (FinishedItem*)lParam;
		addEntry(entry);
		if(BOOLSETTING(BOLD_FINISHED_UPLOADS))
			setDirty();
		updateStatus();
	} else if(wParam == SPEAK_REMOVE) {
		updateStatus();
	} else if(wParam == SPEAK_REMOVE_ALL) {
		ctrlList.DeleteAllItems();
		updateStatus();
	}
	return 0;
}

void FinishedULFrame::addEntry(FinishedItem* entry) {
	TStringList l;
	l.push_back(Text::toT(Util::getFileName(entry->getTarget())));
	l.push_back(Text::toT(Util::formatTime("%Y-%m-%d %H:%M:%S", entry->getTime())));
	l.push_back(Text::toT(Util::getFilePath(entry->getTarget())));
	l.push_back(Text::toT(entry->getUser()));
	l.push_back(Text::toT(entry->getHub()));
	l.push_back(Util::formatBytesW(entry->getSize()));
	l.push_back(Util::formatBytesW(entry->getAvgSpeed()) + _T("/s"));
	totalBytes += entry->getChunkSize();
	totalTime += entry->getMilliSeconds();

	int image = WinUtil::getIconIndex(Text::toT(entry->getTarget()));
	int loc = ctrlList.insert(l, image, (LPARAM)entry);
	ctrlList.EnsureVisible(loc, FALSE);
}

/**
 * @file
 * $Id: FinishedULFrame.cpp,v 1.2 2008/03/10 06:34:23 alexey Exp $
 */
