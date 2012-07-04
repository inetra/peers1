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

#if !defined(UC_PAGE_H)
#define UC_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class UserCommand;

class UCPage : 
  public PeersPropertyPage<IDD_UCPAGE>, 
  public PropPageImpl<UCPage,15>
{
public:
	UCPage() {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_USER_COMMANDS)).c_str());
		SetTitle(title);
	}

	virtual ~UCPage() {
		ctrlCommands.Detach();
		free(title);
	}

	BEGIN_MSG_MAP(UCPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_ADD_MENU, onAddMenu)
		COMMAND_ID_HANDLER(IDC_REMOVE_MENU, onRemoveMenu)
		COMMAND_ID_HANDLER(IDC_CHANGE_MENU, onChangeMenu)
		COMMAND_ID_HANDLER(IDC_MOVE_UP, onMoveUp)
		COMMAND_ID_HANDLER(IDC_MOVE_DOWN, onMoveDown)
		NOTIFY_HANDLER(IDC_MENU_ITEMS, LVN_KEYDOWN, onKeyDown)
		NOTIFY_HANDLER(IDC_MENU_ITEMS, NM_DBLCLK, onDoubleClick)
		NOTIFY_HANDLER(IDC_MENU_ITEMS, LVN_ITEMCHANGED, onItemchangedDirectories)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onAddMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onChangeMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRemoveMenu(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveUp(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMoveDown(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	virtual void write();
	
protected:
	ExListViewCtrl ctrlCommands;

	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	void addEntry(const UserCommand& uc, int pos);
};

#endif // !defined(UC_PAGE_H)

/**
 * @file
 * $Id: UCPage.h,v 1.4 2007/12/08 14:59:18 alexey Exp $
 */
