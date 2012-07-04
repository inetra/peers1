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

#include "UsersFrame.h"
#include "LineDlg.h"
#include "../peers/HubFrameFactory.h"
#include "TextFrame.h"

int UsersFrame::columnIndexes[] = { COLUMN_NICK, COLUMN_HUB, COLUMN_SEEN, COLUMN_DESCRIPTION, COLUMN_SUPERUSER, COLUMN_SPEED_LIMIT, COLUMN_IGNORE, COLUMN_USER_SLOTS }; // !SMT!-S
int UsersFrame::columnSizes[] = { 200, 300, 150, 200, 100, 100,100,100 }; // !SMT!-S
static ResourceManager::Strings columnNames[] = { ResourceManager::AUTO_GRANT, ResourceManager::LAST_HUB, ResourceManager::LAST_SEEN, ResourceManager::DESCRIPTION, ResourceManager::SUPER_USER,
ResourceManager::UPLOAD_SPEED_LIMIT, 
ResourceManager::IGNORE_PRIVATE, 
ResourceManager::SLOTS }; // !SMT!-S

LRESULT UsersFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);

	ctrlUsers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_USERS);
	ctrlUsers.SetExtendedListViewStyle(LVS_EX_LABELTIP | LVS_EX_HEADERDRAGDROP | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
        images.CreateFromImage(IDB_FAVSTATES, 16, 6, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED); // !SMT!-UI
	ctrlUsers.SetImageList(images, LVSIL_SMALL);

	ctrlUsers.SetBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextBkColor(WinUtil::bgColor);
	ctrlUsers.SetTextColor(WinUtil::textColor);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(USERSFRAME_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(USERSFRAME_WIDTHS), COLUMN_LAST);
	
	for(uint8_t j=0; j<COLUMN_LAST; j++) {
		ctrlUsers.InsertColumn(j, CTSTRING_I(columnNames[j]), LVCFMT_LEFT, columnSizes[j], j);
	}
	
        ctrlUsers.setColumnOrderArray(COLUMN_LAST, columnIndexes);
                ctrlUsers.setVisible(SETTING(USERSFRAME_VISIBLE)); // !SMT!-UI
	ctrlUsers.setSortColumn(COLUMN_NICK);

	usersMenu.CreatePopupMenu();
	usersMenu.AppendMenu(MF_STRING, IDC_EDIT, CTSTRING(PROPERTIES));
        usersMenu.AppendMenu(MF_STRING, IDC_SUPER_USER, CTSTRING(SUPER_USER));
        usersMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::speedMenu, CTSTRING(UPLOAD_SPEED_LIMIT)); // !SMT!-S
        usersMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::privateMenu, CTSTRING(IGNORE_PRIVATE)); // !SMT!-PSW
	usersMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, CTSTRING(OPEN_USER_LOG));
	usersMenu.AppendMenu(MF_SEPARATOR);
	appendUserItems(usersMenu, true);
	usersMenu.AppendMenu(MF_SEPARATOR);
	usersMenu.AppendMenu(MF_STRING, IDC_REMOVE, CTSTRING(REMOVE));

	FavoriteManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	FavoriteManager::FavoriteMap ul = FavoriteManager::getInstance()->getFavoriteUsers();
	ctrlUsers.SetRedraw(FALSE);
	for(FavoriteManager::FavoriteMap::const_iterator i = ul.begin(); i != ul.end(); ++i) {
		addUser(i->second);
	}
	ctrlUsers.SetRedraw(TRUE);

	startup = false;

	bHandled = FALSE;
	return TRUE;

}

LRESULT UsersFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (reinterpret_cast<HWND>(wParam) == ctrlUsers && ctrlUsers.GetSelectedCount() > 0 ) { 
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		if(pt.x == -1 && pt.y == -1) {
			WinUtil::getContextMenuPos(ctrlUsers, pt);
		}
	
		checkAdcItems(usersMenu);
		
		tstring x;
		if (ctrlUsers.GetSelectedCount() == 1) {
			x = Text::toT(ctrlUsers.getItemData(WinUtil::getFirstSelectedIndex(ctrlUsers))->user->getFirstNick());
		} else {
			x = _T("");
		}

		if (!x.empty())
			usersMenu.InsertSeparatorFirst(x);
		
                usersMenu.CheckMenuItem(IDC_SUPER_USER, MF_BYCOMMAND | MF_UNCHECKED);
                if (ctrlUsers.GetSelectedCount() == 1) {
                        int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
                        FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
                        FavoriteUser & u = favUsers.find(ctrlUsers.getItemData(i)->user->getCID())->second;
                        if (u.isSet(FavoriteUser::FLAG_SUPERUSER))
                                usersMenu.CheckMenuItem(IDC_SUPER_USER, MF_BYCOMMAND | MF_CHECKED);
                }

                updateSummary(); // !SMT!-UI

                // !SMT!-S
                WinUtil::privateMenu.CheckMenuItem(IDC_PM_NONE, MF_BYCOMMAND | MF_UNCHECKED);
                WinUtil::privateMenu.CheckMenuItem(IDC_PM_IGNORED, MF_BYCOMMAND | MF_UNCHECKED);
                WinUtil::privateMenu.CheckMenuItem(IDC_PM_FREE, MF_BYCOMMAND | MF_UNCHECKED);

                for (int j = 0; j < WinUtil::speedMenu.GetMenuItemCount(); j++)
                   WinUtil::speedMenu.CheckMenuItem(j, MF_BYPOSITION | MF_UNCHECKED);
                if (ctrlUsers.GetSelectedCount() == 1) {
                   int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
                   FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
                   FavoriteUser & u = favUsers.find(ctrlUsers.getItemData(i)->user->getCID())->second;

                   if (u.isSet(FavoriteUser::FLAG_IGNOREPRIVATE))
                      WinUtil::privateMenu.CheckMenuItem(IDC_PM_IGNORED, MF_BYCOMMAND | MF_CHECKED);
                   else if (u.isSet(FavoriteUser::FLAG_FREE_PM_ACCESS))
                      WinUtil::privateMenu.CheckMenuItem(IDC_PM_FREE, MF_BYCOMMAND | MF_CHECKED);
                   else
                      WinUtil::privateMenu.CheckMenuItem(IDC_PM_NONE, MF_BYCOMMAND | MF_CHECKED);

                   int id = IDC_SPEED_NONE;
                   switch (u.getUploadLimit()) {
                      case FavoriteUser::UL_SU:  id = IDC_SPEED_SUPER; break;
                      case FavoriteUser::UL_BAN: id = IDC_SPEED_BAN; break;
                      case FavoriteUser::UL_2:   id = IDC_SPEED_02K; break;
                      case FavoriteUser::UL_5:   id = IDC_SPEED_05K; break;
                      case FavoriteUser::UL_8:   id = IDC_SPEED_08K; break;
                      case FavoriteUser::UL_12:  id = IDC_SPEED_12K; break;
                      case FavoriteUser::UL_16:  id = IDC_SPEED_16K; break;
                      case FavoriteUser::UL_24:  id = IDC_SPEED_24K; break;
                      case FavoriteUser::UL_32:  id = IDC_SPEED_32K; break;
                      case FavoriteUser::UL_64:  id = IDC_SPEED_64K; break;
                      case FavoriteUser::UL_128: id = IDC_SPEED_128K; break;
                      case FavoriteUser::UL_256: id = IDC_SPEED_256K; break;
                   }
                   WinUtil::speedMenu.CheckMenuItem(id, MF_BYCOMMAND | MF_CHECKED);
                }

		usersMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		if (!x.empty())
			usersMenu.RemoveFirstItem();

		return TRUE; 
	}
	bHandled = FALSE;
	return FALSE; 
}
	
void UsersFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
		
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[3];
		ctrlStatus.GetClientRect(sr);
		int tmp = (sr.Width()) > 316 ? 216 : ((sr.Width() > 116) ? sr.Width()-100 : 16);
			
		w[0] = sr.right - tmp;
		w[1] = w[0] + (tmp-16)/2;
		w[2] = w[0] + (tmp-16);
			
		ctrlStatus.SetParts(3, w);
	}
		
	CRect rc = rect;
	ctrlUsers.MoveWindow(rc);
}

LRESULT UsersFrame::onRemove(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ctrlUsers.getItemData(i)->remove();
	}
	return 0;
}

LRESULT UsersFrame::onEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlUsers.GetSelectedCount() == 1) {
		int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
		UserInfo* ui = ctrlUsers.getItemData(i);
		dcassert(i != -1);
		LineDlg dlg;
		dlg.description = TSTRING(DESCRIPTION);
		dlg.title = ui->columns[COLUMN_NICK];
		dlg.line = ui->columns[COLUMN_DESCRIPTION];
		if(dlg.DoModal(m_hWnd)) {
			FavoriteManager::getInstance()->setUserDescription(ui->user, Text::fromT(dlg.line));
			ui->columns[COLUMN_DESCRIPTION] = dlg.line;
			ctrlUsers.updateItem(i);
		}	
	}
	return 0;
}

LRESULT UsersFrame::onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* l = (NMITEMACTIVATE*)pnmh;
	if(!startup && l->iItem != -1 && ((l->uNewState & LVIS_STATEIMAGEMASK) != (l->uOldState & LVIS_STATEIMAGEMASK))) {
		FavoriteManager::getInstance()->setAutoGrant(ctrlUsers.getItemData(l->iItem)->user, ctrlUsers.GetCheckState(l->iItem) != FALSE);
 	}
  	return 0;
}

LRESULT UsersFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
                // !SMT!-UI
                static const int cmd[] = { IDC_GETLIST, IDC_PRIVATEMESSAGE, IDC_MATCH_QUEUE, IDC_EDIT, IDC_OPEN_USER_LOG };
                PostMessage(WM_COMMAND, cmd[SETTING(FAVUSERLIST_DBLCLICK)], 0);
	} else {
		bHandled = FALSE;
	}

	return 0;
}

LRESULT UsersFrame::onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled) {
	NMLVKEYDOWN* kd = (NMLVKEYDOWN*) pnmh;
	switch(kd->wVKey) {
	case VK_DELETE:
		PostMessage(WM_COMMAND, IDC_REMOVE, 0);
		break;
	case VK_RETURN:
		PostMessage(WM_COMMAND, IDC_EDIT, 0);
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

LRESULT UsersFrame::onConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
	for(int i = 0; i < ctrlUsers.GetItemCount(); ++i) {
		UserInfo *ui = ctrlUsers.getItemData(i);
		FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
		const FavoriteUser u = favUsers.find(ui->user->getCID())->second;
		if(u.getUrl().length() > 0)
		{
			HubFrameFactory::openWindow(Text::toT(u.getUrl()),false);
		}
	}
	return 0;
}

void UsersFrame::addUser(const FavoriteUser& aUser) {
	int i = ctrlUsers.insertItem(new UserInfo(aUser), 0);
	bool b = aUser.isSet(FavoriteUser::FLAG_GRANTSLOT);
	ctrlUsers.SetCheckState(i, b);
	updateUser(aUser.getUser());
}

void UsersFrame::updateUser(const UserPtr& aUser) {
	for(int i = 0; i < ctrlUsers.GetItemCount(); ++i) {
		UserInfo *ui = ctrlUsers.getItemData(i);
		if(ui->user == aUser) {
			ui->columns[COLUMN_SEEN] = aUser->isOnline() ? TSTRING(ONLINE) : Text::toT(Util::formatTime("%Y-%m-%d %H:%M", FavoriteManager::getInstance()->getLastSeen(aUser)));

                        // !SMT!-UI
                        int imageIndex = aUser->isOnline()? (aUser->isSet(User::AWAY)? 1:0): 2;
                        if (FavoriteManager::getInstance()->hasBan(aUser) || FavoriteManager::getInstance()->hasIgnore(aUser))
                                imageIndex += 3;
                        ctrlUsers.SetItem(i,0,LVIF_IMAGE, NULL, imageIndex, 0, 0, NULL);

			ctrlUsers.updateItem(i);
		}
	}
}

void UsersFrame::removeUser(const FavoriteUser& aUser) {
	for(int i = 0; i < ctrlUsers.GetItemCount(); ++i) {
		UserInfo *ui = ctrlUsers.getItemData(i);
		if(ui->user == aUser.getUser()) {
			ctrlUsers.DeleteItem(i);
			delete ui;
			return;
		}
	}
}

LRESULT UsersFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(!closed) {
		FavoriteManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
                WinUtil::UnlinkStaticMenus(usersMenu); // !SMT!-S
		closed = true;
		WinUtil::setButtonPressed(IDC_FAVUSERS, false);
		PostMessage(WM_CLOSE);
		return 0;
	} else {
                ctrlUsers.saveHeaderOrder(SettingsManager::USERSFRAME_ORDER, SettingsManager::USERSFRAME_WIDTHS, SettingsManager::USERSFRAME_VISIBLE); // !SMT!-UI
	
		for(int i = 0; i < ctrlUsers.GetItemCount(); ++i) {
			delete ctrlUsers.getItemData(i);
		}

		bHandled = FALSE;
	return 0;
	}
}

void UsersFrame::UserInfo::update(const FavoriteUser& u) {
	columns[COLUMN_NICK] = Text::toT(u.getNick());
	columns[COLUMN_HUB] = user->isOnline() ? WinUtil::getHubNames(u.getUser()).first : Text::toT(u.getUrl());
	columns[COLUMN_SEEN] = user->isOnline() ? TSTRING(ONLINE) : Text::toT(Util::formatTime("%Y-%m-%d %H:%M", u.getLastSeen()));
	columns[COLUMN_DESCRIPTION] = Text::toT(u.getDescription());
        columns[COLUMN_SUPERUSER] = u.isSet(FavoriteUser::FLAG_SUPERUSER)? _T("yes") : _T("no");

        // !SMT!-S
        if (u.isSet(FavoriteUser::FLAG_IGNOREPRIVATE))
            columns[COLUMN_IGNORE] = CTSTRING(IGNORE_PRIVATE);
        else if (u.isSet(FavoriteUser::FLAG_FREE_PM_ACCESS)) 
            columns[COLUMN_IGNORE] = CTSTRING(FREE_PM_ACCESS);
        else
            columns[COLUMN_IGNORE] = _T("");
        columns[COLUMN_SPEED_LIMIT] = Text::toT(FavoriteUser::GetLimitText(u.getUploadLimit()));
		//[+]PPA
		columns[COLUMN_USER_SLOTS] = Text::toT(Util::toString(u.getUser()->getCountSlots()));
}

LRESULT UsersFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	if(wParam == USER_UPDATED) {
		UserInfoBase* uib = (UserInfoBase*)lParam;
		updateUser(uib->user);
		delete uib;
	}
	return 0;
}
			
LRESULT UsersFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ctrlUsers.GetSelectedCount() == 1) {
		int i = ctrlUsers.GetNextItem(-1, LVNI_SELECTED);
		UserInfo* ui = ctrlUsers.getItemData(i);
		dcassert(i != -1);

		StringMap params;
		params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(ui->getUser()->getCID()));
		params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(ui->getUser()->getCID()));
		params["userCID"] = ui->getUser()->getCID().toBase32(); 
		params["userNI"] = ui->getUser()->getFirstNick();
		params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();

		string file = Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false));
		if(Util::fileExists(file)) {
                        if(BOOLSETTING(OPEN_LOGS_INTERNAL) == false) {
			ShellExecute(NULL, NULL, Text::toT(file).c_str(), NULL, NULL, SW_SHOWNORMAL);
		} else {
                                TextFrame::openWindow(Text::toT(file).c_str());
                        }
                } else {
			MessageBox(CTSTRING(NO_LOG_FOR_USER), CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
		}	
	}
		return 0;
}

LRESULT UsersFrame::onSuperUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
   int i = -1;
   while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
           UserInfo *ui = ctrlUsers.getItemData(i);
           FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
           FavoriteUser & u = favUsers.find(ui->user->getCID())->second;
           ui->columns[COLUMN_SUPERUSER] = u.isSet(FavoriteUser::FLAG_SUPERUSER)? _T("no") : _T("yes");
           FavoriteManager::getInstance()->setSuperUser(u.getUser(), !u.isSet(FavoriteUser::FLAG_SUPERUSER));
           ctrlUsers.updateItem(i);
   }
   return 0;
}

void UsersFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	bool refresh = false;
	if(ctrlUsers.GetBkColor() != WinUtil::bgColor) {
		ctrlUsers.SetBkColor(WinUtil::bgColor);
		ctrlUsers.SetTextBkColor(WinUtil::bgColor);
		refresh = true;
	}
	if(ctrlUsers.GetTextColor() != WinUtil::textColor) {
		ctrlUsers.SetTextColor(WinUtil::textColor);
		refresh = true;
	}
	if(refresh == true) {
		RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}
}

// !SMT!-S
LRESULT UsersFrame::onIgnorePrivate(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
   int i = -1;
   while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
      UserInfo *ui = ctrlUsers.getItemData(i);
      FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
      FavoriteUser & u = favUsers.find(ui->user->getCID())->second;

      FavoriteManager::getInstance()->setIgnorePrivate(u.getUser(), false);
      FavoriteManager::getInstance()->setFreePM(u.getUser(), false);
      ui->columns[COLUMN_IGNORE] = _T("");

      if (wID == IDC_PM_IGNORED) {
         ui->columns[COLUMN_IGNORE] = CTSTRING(IGNORE_PRIVATE);
         FavoriteManager::getInstance()->setIgnorePrivate(u.getUser(), true);
      }

      if (wID == IDC_PM_FREE) {
         ui->columns[COLUMN_IGNORE] = CTSTRING(FREE_PM_ACCESS);
         FavoriteManager::getInstance()->setFreePM(u.getUser(), true);
      }

      updateUser(ui->user);
      ctrlUsers.updateItem(i);
   }
   return 0;
}

// !SMT!-S
LRESULT UsersFrame::onSetUserLimit(WORD /* wNotifyCode */, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
      FavoriteUser::UPLOAD_LIMIT lim;
      switch (wID) {
         case IDC_SPEED_NONE:  lim = FavoriteUser::UL_NONE; break;
         case IDC_SPEED_SUPER: lim = FavoriteUser::UL_SU;   break;
         case IDC_SPEED_BAN:   lim = FavoriteUser::UL_BAN;  break;
         case IDC_SPEED_02K:   lim = FavoriteUser::UL_2;    break;
         case IDC_SPEED_05K:   lim = FavoriteUser::UL_5;    break;
         case IDC_SPEED_08K:   lim = FavoriteUser::UL_8;    break;
         case IDC_SPEED_12K:   lim = FavoriteUser::UL_12;   break;
         case IDC_SPEED_16K:   lim = FavoriteUser::UL_16;   break;
         case IDC_SPEED_24K:   lim = FavoriteUser::UL_24;   break;
         case IDC_SPEED_32K:   lim = FavoriteUser::UL_32;   break;
         case IDC_SPEED_64K:   lim = FavoriteUser::UL_64;   break;
      case IDC_SPEED_128K:  lim = FavoriteUser::UL_128;  break;
      case IDC_SPEED_256K:  lim = FavoriteUser::UL_256;  break;
         default: return 0;
      }
   int i = -1;
   while( (i = ctrlUsers.GetNextItem(i, LVNI_SELECTED)) != -1) {
      UserInfo *ui = ctrlUsers.getItemData(i);
      FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
      FavoriteUser & u = favUsers.find(ui->user->getCID())->second;
      FavoriteManager::getInstance()->setUploadLimit(u.getUser(), lim);
	  ui->columns[COLUMN_SPEED_LIMIT] = Text::toT(FavoriteUser::GetLimitText(lim));
      updateUser(ui->user);
      ctrlUsers.updateItem(i);
   }
   return 0;
}

/**
 * @file
 * $Id: UsersFrame.cpp,v 1.3 2008/03/10 08:47:07 alexey Exp $
 */
