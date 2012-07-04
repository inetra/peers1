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

#if !defined(USERS_FRAME_H)
#define USERS_FRAME_H

#include "TypedListViewCtrl.h"
#include "UserInfoBase.h"
#include "../client/FavoriteManager.h"

class UsersFrame : 
  public MDITabChildWindowImpl<UsersFrame, IDR_USERS>, 
  public StaticFrame<UsersFrame, ResourceManager::FAVORITE_USERS, IDC_FAVUSERS>,
  private FavoriteManagerListener, 
  public UserInfoBaseHandler<UsersFrame>, 
  private SettingsManagerListener {
public:
	
	UsersFrame() : closed(false), startup(true) { }
	virtual ~UsersFrame() { images.Destroy(); }

	DECLARE_FRAME_WND_CLASS_EX(_T("UsersFrame"), IDR_USERS, 0, COLOR_3DFACE);
		
	typedef MDITabChildWindowImpl<UsersFrame, IDR_USERS> baseClass;
	typedef UserInfoBaseHandler<UsersFrame> uibBase;

	BEGIN_MSG_MAP(UsersFrame)
		NOTIFY_HANDLER(IDC_USERS, LVN_GETDISPINFO, ctrlUsers.onGetDispInfo)
		NOTIFY_HANDLER(IDC_USERS, LVN_COLUMNCLICK, ctrlUsers.onColumnClick)
		NOTIFY_HANDLER(IDC_USERS, LVN_ITEMCHANGED, onItemChanged)
		NOTIFY_HANDLER(IDC_USERS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_USERS, NM_DBLCLK, onDoubleClick)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		COMMAND_ID_HANDLER(IDC_REMOVE, onRemove)
		COMMAND_ID_HANDLER(IDC_EDIT, onEdit)
		COMMAND_ID_HANDLER(IDC_CONNECT, onConnect)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
                COMMAND_ID_HANDLER(IDC_SUPER_USER, onSuperUser)

                // !SMT!-S
                COMMAND_ID_HANDLER(IDC_PM_NONE, onIgnorePrivate)
                COMMAND_ID_HANDLER(IDC_PM_IGNORED, onIgnorePrivate)
                COMMAND_ID_HANDLER(IDC_PM_FREE, onIgnorePrivate)
                COMMAND_ID_HANDLER(IDC_SPEED_NONE, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_SUPER,onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_BAN, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_02K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_05K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_08K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_12K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_16K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_24K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_32K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_64K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_128K, onSetUserLimit)
                COMMAND_ID_HANDLER(IDC_SPEED_256K, onSetUserLimit)

		CHAIN_MSG_MAP(uibBase)
		CHAIN_MSG_MAP(baseClass)
	END_MSG_MAP()
		
	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onRemove(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onEdit(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT onSuperUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

        // !SMT!-S
        LRESULT onIgnorePrivate(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onSetUserLimit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void UpdateLayout(BOOL bResizeBars = TRUE);

	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	
	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlUsers.SetFocus();
		return 0;
	}

private:
	class UserInfo;
public:
	TypedListViewCtrl<UserInfo, IDC_USERS>& getUserList() { return ctrlUsers; }
private:
	enum {
		COLUMN_FIRST,
		COLUMN_NICK = COLUMN_FIRST,
		COLUMN_HUB,
		COLUMN_SEEN,
		COLUMN_DESCRIPTION,
                COLUMN_SUPERUSER,
                COLUMN_SPEED_LIMIT, // !SMT!-S
                COLUMN_IGNORE, // !SMT!-S
				COLUMN_USER_SLOTS, //[+]ppa
		COLUMN_LAST
	};

	enum {
		USER_UPDATED
	};

	class UserInfo : public UserInfoBase {
	public:
		UserInfo(const FavoriteUser& u) : UserInfoBase(u.getUser()) { 
			update(u);
		}

		const tstring& getText(int col) const {
			return columns[col];
		}

		static int compareItems(const UserInfo* a, const UserInfo* b, int col) {
			return Util::DefaultSort(a->columns[col].c_str(), b->columns[col].c_str());
		}
		
		int imageIndex() const { return 2; }

		void remove() { FavoriteManager::getInstance()->removeFavoriteUser(user); }

		void update(const FavoriteUser& u);

		tstring columns[COLUMN_LAST];
	};

	CStatusBarCtrl ctrlStatus;
	OMenu usersMenu;
	
	TypedListViewCtrl<UserInfo, IDC_USERS> ctrlUsers;
	CImageList images;	

	bool closed;
	
	bool startup;
	static int columnSizes[COLUMN_LAST];
	static int columnIndexes[COLUMN_LAST];

	// FavoriteManagerListener
	virtual void on(UserAdded, const FavoriteUser& aUser) throw() { addUser(aUser); }
	virtual void on(UserRemoved, const FavoriteUser& aUser) throw() { removeUser(aUser); }
	virtual void on(StatusChanged, const UserPtr& aUser) throw() { PostMessage(WM_SPEAKER, (WPARAM)USER_UPDATED, (LPARAM)new UserInfoBase(aUser)); }

	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();

	void addUser(const FavoriteUser& aUser);
	void updateUser(const UserPtr& aUser);
	void removeUser(const FavoriteUser& aUser);
};

#endif // !defined(USERS_FRAME_H)

/**
 * @file
 * $Id: UsersFrame.h,v 1.5 2008/03/10 07:44:31 alexey Exp $
 */
