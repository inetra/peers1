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
#include "../client/FavoriteManager.h"
#include "../client/QueueManager.h"
#include "WaitingUsersFrame.h"
#include "../peers/PrivateFrameFactory.h"
#include "UserInfoBase.h"

#include "BarShader.h"

int WaitingUsersFrame::columnSizes[] = { 250, 100, 75, 75, 75, 75, 100, 100, 100,100,150, 75 }; // !SMT!-UI
int WaitingUsersFrame::columnIndexes[] = { COLUMN_FILE, COLUMN_PATH, COLUMN_NICK, COLUMN_HUB, COLUMN_TRANSFERRED, COLUMN_SIZE, COLUMN_ADDED, COLUMN_WAITING,
COLUMN_LOCATION, COLUMN_IP, // !SMT!-IP
#ifdef PPA_INCLUDE_DNS
 COLUMN_DNS, // !SMT!-IP
#endif
COLUMN_SLOTS, COLUMN_SHARE // !SMT!-UI
};
static ResourceManager::Strings columnNames[] = { ResourceManager::FILENAME, ResourceManager::PATH, ResourceManager::NICK, 
        ResourceManager::HUB, ResourceManager::TRANSFERRED, ResourceManager::SIZE, ResourceManager::ADDED, ResourceManager::WAITING_TIME,
        ResourceManager::LOCATION_BARE,ResourceManager::IP_BARE,
#ifdef PPA_INCLUDE_DNS
 ResourceManager::DNS_BARE, // !SMT!-IP
#endif
ResourceManager::SLOTS,ResourceManager::SHARED // !SMT!-UI
};

LRESULT WaitingUsersFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  m_header.Create(m_hWnd);
  m_header.addWords(TSTRING(WAITING_USERS));
	showTree = BOOLSETTING(UPLOADQUEUEFRAME_SHOW_TREE);

	// status bar
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_UPLOAD_QUEUE);

	ctrlList.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT | 0x00010000 | LVS_EX_INFOTIP);
	ctrlQueued.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		TVS_HASBUTTONS | TVS_LINESATROOT | TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_DISABLEDRAGDROP, 
		 WS_EX_CLIENTEDGE, IDC_DIRECTORIES);
	
	ctrlQueued.SetImageList(WinUtil::fileImages, TVSIL_NORMAL);
	ctrlList.SetImageList(WinUtil::fileImages, LVSIL_SMALL);

	m_nProportionalPos = 2500;
	SetSplitterPanes(ctrlQueued.m_hWnd, ctrlList.m_hWnd);

	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(UPLOADQUEUEFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(UPLOADQUEUEFRAME_WIDTHS), COLUMN_LAST);

	// column names, sizes
	for (uint8_t j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_TRANSFERRED || j == COLUMN_SIZE) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CTSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
		
	ctrlList.setColumnOrderArray(COLUMN_LAST, columnIndexes);
	ctrlList.setSortColumn(COLUMN_NICK);
	ctrlList.setVisible(SETTING(UPLOADQUEUEFRAME_VISIBLE));
	
	// colors
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);

	ctrlQueued.SetBkColor(WinUtil::bgColor);
	ctrlQueued.SetTextColor(WinUtil::textColor);
	
	ctrlShowTree.Create(ctrlStatus.m_hWnd, rcDefault, _T("+/-"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
	ctrlShowTree.SetButtonStyle(BS_AUTOCHECKBOX, false);
	ctrlShowTree.SetCheck(showTree);
	showTreeContainer.SubclassWindow(ctrlShowTree.m_hWnd);

	// Create context menu
        /* !SMT!-UI
	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));
        */

	contextMenu.CreatePopupMenu();
	contextMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
        contextMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU)); // !SMT!-UI
        contextMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::userSummaryMenu, CTSTRING(USER_SUMMARY)); // !SMT!-UI
	contextMenu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
	contextMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
	contextMenu.AppendMenu(MF_SEPARATOR);
	contextMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

    memzero(statusSizes, sizeof(statusSizes));
	statusSizes[0] = 16;
	ctrlStatus.SetParts(4, statusSizes);
	UpdateLayout();

	UploadManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);
	// Load all searches
	LoadAll();

	bHandled = FALSE;
	return TRUE;
}

LRESULT WaitingUsersFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		UploadManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
		WinUtil::setButtonPressed(IDC_UPLOAD_QUEUE, false);
		closed = true;
		PostMessage(WM_CLOSE);
		return 0;
	} else {
		HTREEITEM userNode = ctrlQueued.GetRootItem();

		while (userNode) {
			delete reinterpret_cast<UserItem *>(ctrlQueued.GetItemData(userNode));
			userNode = ctrlQueued.GetNextSiblingItem(userNode);
		}
		ctrlList.DeleteAllItems();
		UQFUsers.clear();
		SettingsManager::getInstance()->set(SettingsManager::UPLOADQUEUEFRAME_SHOW_TREE, ctrlShowTree.GetCheck() == BST_CHECKED);
		ctrlList.saveHeaderOrder(SettingsManager::UPLOADQUEUEFRAME_ORDER, SettingsManager::UPLOADQUEUEFRAME_WIDTHS,
			SettingsManager::UPLOADQUEUEFRAME_VISIBLE);

		bHandled = FALSE;
		return 0;
	}
}

void WaitingUsersFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
        m_header.updateLayout(rect.left, rect.top, rect.right);
        rect.top += m_header.getPreferredHeight();

	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[4];
		ctrlStatus.GetClientRect(sr);
		w[3] = sr.right - 16;
#define setw(x) w[x] = max(w[x+1] - statusSizes[x], 0)
		setw(2); setw(1);

		w[0] = 16;

		ctrlStatus.SetParts(4, w);

		ctrlStatus.GetRect(0, sr);
		ctrlShowTree.MoveWindow(sr);
	}
	if(showTree) {
		if(GetSinglePaneMode() != SPLIT_PANE_NONE) {
			SetSinglePaneMode(SPLIT_PANE_NONE);
		}
	} else {
		if(GetSinglePaneMode() != SPLIT_PANE_RIGHT) {
			SetSinglePaneMode(SPLIT_PANE_RIGHT);
		}
	}
	CRect rc = rect;
	SetSplitterRect(rc);
}

LRESULT WaitingUsersFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			QueueManager::getInstance()->addList(User, QueueItem::FLAG_CLIENT_VIEW);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			QueueManager::getInstance()->addList(((UploadQueueItem*)ctrlList.getItemData(i))->User, QueueItem::FLAG_CLIENT_VIEW);
		}
	}
	return 0;
}

LRESULT WaitingUsersFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->clearUserFiles(User);
		}
	} else {
		int i = -1;
		UserList RemoveUsers;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			// Ok let's cheat here, if you try to remove more users here is not working :(
			RemoveUsers.push_back(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
		for(UserList::const_iterator i = RemoveUsers.begin(); i != RemoveUsers.end(); ++i) {
			UploadManager::getInstance()->clearUserFiles(*i);
		}
	}
	updateStatus();
	return 0;
}

LRESULT WaitingUsersFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	RECT rc;
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	ctrlList.GetHeader().GetWindowRect(&rc);
	if(PtInRect(&rc, pt)){
		ctrlList.showMenu(pt);
		return TRUE;
	}
		
        WinUtil::clearSummaryMenu(); // !SMT!-UI

	if(reinterpret_cast<HWND>(wParam) == ctrlList && ctrlList.GetSelectedCount() > 0) {
     	if(pt.x == -1 && pt.y == -1) {
    		WinUtil::getContextMenuPos(ctrlList, pt);
    	}
		usingUserMenu = false;

                // !SMT!-UI
                int i = -1;
                while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
                        UserInfoBase(((UploadQueueItem*)ctrlList.getItemData(i))->User).addSummary();
                }

		contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
		return TRUE;
	} else if(reinterpret_cast<HWND>(wParam) == ctrlQueued && ctrlQueued.GetSelectedItem() != NULL) {
     	if(pt.x == -1 && pt.y == -1) {
    		WinUtil::getContextMenuPos(ctrlQueued, pt);
    	} else {
			UINT a = 0;
    		ctrlQueued.ScreenToClient(&pt);
			HTREEITEM ht = ctrlQueued.HitTest(pt, &a);
			if(ht != NULL && ht != ctrlQueued.GetSelectedItem())
				ctrlQueued.SelectItem(ht);
    
			ctrlQueued.ClientToScreen(&pt);
        }
        usingUserMenu = true;

                // !SMT!-UI
                UserPtr User = getSelectedUser();
                if(User) {
                        UserInfoBase(User).addSummary();
                }
			contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
			return TRUE;
	}	
	return FALSE; 
}

LRESULT WaitingUsersFrame::onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			PrivateFrameFactory::openWindow(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			PrivateFrameFactory::openWindow(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
}

LRESULT WaitingUsersFrame::onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->reserveSlot(User, 600);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->reserveSlot(((UploadQueueItem*)ctrlList.getItemData(i))->User, 600);
		}
	}
	return 0; 
}

LRESULT WaitingUsersFrame::onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->reserveSlot(User, 3600);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->reserveSlot(((UploadQueueItem*)ctrlList.getItemData(i))->User, 3600);
		}
	}
	return 0;
}

LRESULT WaitingUsersFrame::onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->reserveSlot(User, 24*3600);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->reserveSlot(((UploadQueueItem*)ctrlList.getItemData(i))->User, 24*3600);
		}
	}
	return 0;
}

LRESULT WaitingUsersFrame::onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->reserveSlot(User, 7*24*3600);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->reserveSlot(((UploadQueueItem*)ctrlList.getItemData(i))->User, 7*24*3600);
		}
	}
	return 0;
}

// !SMT!-UI
LRESULT WaitingUsersFrame::onGrantSlotPeriod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        uint32_t time = UserInfoBase::inputSlotTime();
        if (!time) return 0;
        if(usingUserMenu) {
                UserPtr User = getSelectedUser();
                if(User) {
                        UploadManager::getInstance()->reserveSlot(User, time);
                }
        } else {
                int i = -1;
                while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
                        UploadManager::getInstance()->reserveSlot(((UploadQueueItem*)ctrlList.getItemData(i))->User, time);
                }
        }
        return 0;
}

LRESULT WaitingUsersFrame::onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			UploadManager::getInstance()->unreserveSlot(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			UploadManager::getInstance()->unreserveSlot(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
}

LRESULT WaitingUsersFrame::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(usingUserMenu) {
		UserPtr User = getSelectedUser();
		if(User) {
			FavoriteManager::getInstance()->addFavoriteUser(User);
		}
	} else {
		int i = -1;
		while((i = ctrlList.GetNextItem(i, LVNI_SELECTED)) != -1) {
			FavoriteManager::getInstance()->addFavoriteUser(((UploadQueueItem*)ctrlList.getItemData(i))->User);
		}
	}
	return 0;
}

void WaitingUsersFrame::LoadAll() {
	ctrlList.SetRedraw(FALSE);
	ctrlQueued.SetRedraw(FALSE);	
	
	HTREEITEM userNode = ctrlQueued.GetRootItem();
	while (userNode) {
		delete reinterpret_cast<UserItem *>(ctrlQueued.GetItemData(userNode));
		userNode = ctrlQueued.GetNextSiblingItem(userNode);
	}
	ctrlList.DeleteAllItems();
	ctrlQueued.DeleteAllItems();
	UQFUsers.clear();

	// Load queue
	UploadQueueItem::UserMap users = UploadManager::getInstance()->getWaitingUsers();
	for(UploadQueueItem::UserMapIter uit = users.begin(); uit != users.end(); ++uit) {
		UQFUsers.push_back(uit->first);
		ctrlQueued.InsertItem(TVIF_PARAM | TVIF_TEXT, (Text::toT(uit->first->getFirstNick()) + _T(" - ") + WinUtil::getHubNames(uit->first).first).c_str(), 
			0, 0, 0, 0, (LPARAM)(new UserItem(uit->first)), TVI_ROOT, TVI_LAST);
		for(UploadQueueItem::Iter i = uit->second.begin(); i != uit->second.end(); ++i) {
			AddFile(*i);
		}
	}
	ctrlList.resort();
	ctrlList.SetRedraw(TRUE);
	ctrlQueued.SetRedraw(TRUE);
	ctrlQueued.Invalidate(); 
	updateStatus();
}

void WaitingUsersFrame::RemoveUser(const UserPtr& aUser) {
	HTREEITEM userNode = ctrlQueued.GetRootItem();

	for(UserList::iterator i = UQFUsers.begin(); i != UQFUsers.end(); ++i) {
		if(*i == aUser) {
			UQFUsers.erase(i);
			break;
		}
	}

	while(userNode) {
		UserItem *u = reinterpret_cast<UserItem *>(ctrlQueued.GetItemData(userNode));
		if (aUser == u->u) {
			delete u;
			ctrlQueued.DeleteItem(userNode);
			return;
		}
		userNode = ctrlQueued.GetNextSiblingItem(userNode);
	}
	updateStatus();
}

LRESULT WaitingUsersFrame::onItemChanged(int /*idCtrl*/, LPNMHDR /* pnmh */, BOOL& /*bHandled*/) {
	HTREEITEM userNode = ctrlQueued.GetSelectedItem();

	while(userNode) {
		ctrlList.DeleteAllItems();
		UserItem *u = reinterpret_cast<UserItem *>(ctrlQueued.GetItemData(userNode));
		UploadQueueItem::UserMap users = UploadManager::getInstance()->getWaitingUsers();
		for (UploadQueueItem::UserMapIter uit = users.begin(); uit != users.end(); ++uit) {
			if(uit->first == u->u) {
				ctrlList.SetRedraw(FALSE);
				ctrlQueued.SetRedraw(FALSE);
				for(UploadQueueItem::Iter i = uit->second.begin(); i != uit->second.end(); ++i) {
					AddFile(*i);
				}
				ctrlList.resort();
				ctrlList.SetRedraw(TRUE);
				ctrlQueued.SetRedraw(TRUE);
				ctrlQueued.Invalidate(); 
				updateStatus();
				return 0;
			}
		}
		userNode = ctrlQueued.GetNextSiblingItem(userNode);
	}
	return 0;
}

void WaitingUsersFrame::AddFile(UploadQueueItem* aUQI) { 
	HTREEITEM userNode = ctrlQueued.GetRootItem();
	bool add = true;

	HTREEITEM selNode = ctrlQueued.GetSelectedItem();

	if(userNode) {
		for(UserList::const_iterator i = UQFUsers.begin(); i != UQFUsers.end(); ++i) {
			if(*i == aUQI->User) {
				add = false;
				break;
			}
		}
	}
	if(add) {
		UQFUsers.push_back(aUQI->User);
		userNode = ctrlQueued.InsertItem(TVIF_PARAM | TVIF_TEXT, (Text::toT(aUQI->User->getFirstNick()) + _T(" - ") + WinUtil::getHubNames(aUQI->User).first).c_str(), 
			0, 0, 0, 0, (LPARAM)(new UserItem(aUQI->User)), TVI_ROOT, TVI_LAST);
	}	
	if(selNode) {
		TCHAR selBuf[256];
		ctrlQueued.GetItemText(selNode, selBuf, 255);
		if(_tcscmp(selBuf, (Text::toT(aUQI->User->getFirstNick()) + _T(" - ") + WinUtil::getHubNames(aUQI->User).first).c_str()) != 0) {
			return;
		}
	}
	aUQI->update();
	aUQI->icon = WinUtil::getIconIndex(Text::toT(aUQI->File));
	ctrlList.insertItem(ctrlList.GetItemCount(), aUQI, aUQI->icon);
}

HTREEITEM WaitingUsersFrame::GetParentItem() {
	HTREEITEM item = ctrlQueued.GetSelectedItem(), parent = ctrlQueued.GetParentItem(item);
	parent = parent?parent:item;
	ctrlQueued.SelectItem(parent);
	return parent;
}

void WaitingUsersFrame::updateStatus() {
	if(ctrlStatus.IsWindow()) {

		int cnt = ctrlList.GetItemCount();
		int users = ctrlQueued.GetCount();	

		tstring tmp[2];
		if(showTree) {
			tmp[0] = TSTRING(USERS) + _T(": ") + Util::toStringW(users);
		} else {
			tmp[0] = _T("");
		}    		  
		tmp[1] = TSTRING(ITEMS) + _T(": ") + Util::toStringW(cnt);
		bool u = false;

		for(int i = 1; i < 3; i++) {
			int w = WinUtil::getTextWidth(tmp[i-1], ctrlStatus.m_hWnd);
				
			if(statusSizes[i] < w) {
				statusSizes[i] = w + 50;
				u = true;
			}
			ctrlStatus.SetText(i+1, tmp[i-1].c_str());
		}

		if(u)
			UpdateLayout(TRUE);
	}
}


void UploadQueueItem::update() {
	setText(COLUMN_FILE, Text::toT(Util::getFileName(File)));
	setText(COLUMN_PATH, Text::toT(Util::getFilePath(File)));
	setText(COLUMN_NICK, Text::toT(User->getFirstNick()));
	setText(COLUMN_HUB, WinUtil::getHubNames(User).first);
	setText(COLUMN_TRANSFERRED, Util::formatBytesW(pos) + _T(" (") + Util::toStringW((double)pos*100.0/(double)size) + _T("%)"));
	setText(COLUMN_SIZE, Util::formatBytesW(size));
	setText(COLUMN_ADDED, Text::toT(Util::formatTime("%Y-%m-%d %H:%M", iTime)));
	setText(COLUMN_WAITING, Util::formatSeconds(GET_TIME() - iTime));
	setText(COLUMN_SHARE, Text::toT(Util::formatBytes(User->getBytesShared()))); //[+]PPA
	setText(COLUMN_SLOTS, Text::toT(Util::toString(User->getCountSlots()))); //[+]PPA
        // !SMT!-IP
        if (flagImage == -1) {
                flagImage = WinUtil::getFlagImage(country);
        }
        setText(COLUMN_IP, Text::toT(ip));
        setText(COLUMN_LOCATION, Text::toT(country));
#ifdef PPA_INCLUDE_DNS
       setText(COLUMN_DNS, Text::toT(dns)); // todo: paint later if not resolved yet
#endif
}

LRESULT WaitingUsersFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	ctrlList.SetRedraw(FALSE);
	if(wParam == REMOVE_ITEM) {
		UploadQueueItem* i = (UploadQueueItem*)lParam;
		ctrlList.deleteItem(i);
		updateStatus();
		i->dec();
		if(BOOLSETTING(BOLD_WAITING_USERS))
			setDirty();		
	} else if(wParam == REMOVE) {
		RemoveUser(((UserInfoBase*)lParam)->user);
		updateStatus();
		delete (UserInfoBase*)lParam;
		if(BOOLSETTING(BOLD_WAITING_USERS))
			setDirty();		
	} else if(wParam == ADD_ITEM) {
		AddFile((UploadQueueItem*)lParam);
		updateStatus();
		ctrlList.resort();
		if(BOOLSETTING(BOLD_WAITING_USERS))
			setDirty();		
	} else if(wParam == UPDATE_ITEMS) {
		int j = ctrlList.GetItemCount();
		int64_t itime = GET_TIME();
		for(int i = 0; i < j; i++) {
			UploadQueueItem* UQI = ctrlList.getItemData(i);
			UQI->setText(COLUMN_TRANSFERRED, Util::formatBytesW(UQI->pos) + _T(" (") + Util::toStringW((double)UQI->pos*100.0/(double)UQI->size) + _T("%)"));
			UQI->setText(COLUMN_WAITING, Util::formatSeconds(itime - UQI->iTime));
			ctrlList.updateItem(i);
		}
	}
	ctrlList.SetRedraw(TRUE);
	return 0;
}

void WaitingUsersFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlList.GetBkColor() != WinUtil::bgColor) {
		ctrlList.SetBkColor(WinUtil::bgColor);
		ctrlList.SetTextBkColor(WinUtil::bgColor);
		ctrlQueued.SetBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlList.GetTextColor() != WinUtil::textColor) {
		ctrlList.SetTextColor(WinUtil::textColor);
		ctrlQueued.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}
LRESULT WaitingUsersFrame::onCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	if(!BOOLSETTING(SHOW_PROGRESS_BARS)
				 || g_RunningUnderWine //[+]PPA
		) {
		bHandled = FALSE;
		return 0;
	}

	CRect rc;
	LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)pnmh;
		UploadQueueItem *ii = (UploadQueueItem*)cd->nmcd.lItemlParam;

	switch(cd->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		return CDRF_NOTIFYSUBITEMDRAW;

	case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
		{
		// Let's draw a box if needed...
		if(ctrlList.findColumn(cd->iSubItem) == COLUMN_TRANSFERRED) {
			// draw something nice...
				TCHAR buf[256];

				ctrlList.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
				buf[255] = 0;

				ctrlList.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
				// Real rc, the original one.
				CRect real_rc = rc;
				// We need to offset the current rc to (0, 0) to paint on the New dc
				rc.MoveToXY(0, 0);

				// Text rect
				CRect rc2 = rc;
                rc2.left += 6; // indented with 6 pixels
				rc2.right -= 2; // and without messing with the border of the cell

				// Set references
				CDC cdc;
				cdc.CreateCompatibleDC(cd->nmcd.hdc);
				HBITMAP hBmp = CreateCompatibleBitmap(cd->nmcd.hdc,  real_rc.Width(),  real_rc.Height());
				HBITMAP pOldBmp = cdc.SelectBitmap(hBmp);
				HDC& dc = cdc.m_hDC;

				HFONT oldFont = (HFONT)SelectObject(dc, WinUtil::font);
				SetBkMode(dc, TRANSPARENT);

				CBarShader statusBar(rc.bottom - rc.top, rc.right - rc.left, RGB(150, 0, 0), ii->size);
				statusBar.FillRange(0, ii->pos, RGB(222, 160, 0));
				statusBar.Draw(cdc, rc.top, rc.left, SETTING(PROGRESS_3DDEPTH));

				SetTextColor(dc, SETTING(PROGRESS_TEXT_COLOR_UP));
                ::ExtTextOut(dc, rc2.left, rc2.top + (rc2.Height() - WinUtil::getTextHeight(dc) - 1)/2, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);

				SelectObject(dc, oldFont);
				
				BitBlt(cd->nmcd.hdc, real_rc.left, real_rc.top, real_rc.Width(), real_rc.Height(), dc, 0, 0, SRCCOPY);

				DeleteObject(cdc.SelectBitmap(pOldBmp));
				return CDRF_SKIPDEFAULT;	
		}

                WinUtil::getUserColor(ii->User, cd->clrText, cd->clrTextBk); // !SMT!-UI

                // !SMT!-IP
                if(/* [-]PPA BOOLSETTING(GET_USER_COUNTRY) && */
                       (ctrlList.findColumn(cd->iSubItem) == COLUMN_LOCATION)) {
                        ctrlList.GetSubItemRect((int)cd->nmcd.dwItemSpec, cd->iSubItem, LVIR_BOUNDS, rc);
                        COLORREF color;
                        if (ctrlList.GetItemState((int)cd->nmcd.dwItemSpec, LVIS_SELECTED) & LVIS_SELECTED) {
                                if(ctrlList.m_hWnd == ::GetFocus()) {
                                        color = GetSysColor(COLOR_HIGHLIGHT);
                                        SetBkColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHT));
                                        SetTextColor(cd->nmcd.hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                                } else {
                                        color = GetBkColor(cd->nmcd.hdc);
                                        SetBkColor(cd->nmcd.hdc, color);
                                }
                        } else {
                                color = WinUtil::bgColor;
                                SetBkColor(cd->nmcd.hdc, cd->clrTextBk);
                                SetTextColor(cd->nmcd.hdc, cd->clrText);
                        }
                        CRect rc2 = rc;
                        rc2.left += 2;
                        HGDIOBJ oldpen = ::SelectObject(cd->nmcd.hdc, CreatePen(PS_SOLID,0, color));
                        HGDIOBJ oldbr = ::SelectObject(cd->nmcd.hdc, CreateSolidBrush(color));
                        Rectangle(cd->nmcd.hdc,rc.left, rc.top, rc.right, rc.bottom);

                        DeleteObject(::SelectObject(cd->nmcd.hdc, oldpen));
                        DeleteObject(::SelectObject(cd->nmcd.hdc, oldbr));

                        TCHAR buf[256];
                        ctrlList.GetItemText((int)cd->nmcd.dwItemSpec, cd->iSubItem, buf, 255);
                        buf[255] = 0;
                        if(_tcslen(buf) > 0) {
                                LONG top = rc2.top + (rc2.Height() - 15)/2;
                                if((top - rc2.top) < 2)
                                        top = rc2.top + 1;

                                POINT p = { rc2.left, top };
                                WinUtil::flagImages.Draw(cd->nmcd.hdc, ii->flagImage, p, LVSIL_SMALL);
                                top = rc2.top + (rc2.Height() - WinUtil::getTextHeight(cd->nmcd.hdc) - 1)/2;
                                ::ExtTextOut(cd->nmcd.hdc, rc2.left + 30, top + 1, ETO_CLIPPED, rc2, buf, _tcslen(buf), NULL);
                                return CDRF_SKIPDEFAULT;
                        }
                }
                } //[+]PPA
		// Fall through
	default:
		return CDRF_DODEFAULT;
	}
}

/**
 * @file
 * $Id: WaitingUsersFrame.cpp,v 1.5 2008/03/10 06:36:07 alexey Exp $
 */
