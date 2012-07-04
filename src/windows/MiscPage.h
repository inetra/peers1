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

#if !defined(MISC_PAGE_H)
#define MISC_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include <atlctrlx.h>

#include "ExListViewCtrl.h"

class MiscPage : 
  public PeersPropertyPage<IDD_MISCPAGE>, 
  public PropPageImpl<MiscPage,23>
{
public:
	MiscPage() {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_MISC)).c_str());
		SetTitle(title);
	}
	~MiscPage() { 
		free(title);
		ignoreListCtrl.Detach();
	}

	BEGIN_MSG_MAP(MiscPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_TIME_AWAY, onFixControls)
		COMMAND_ID_HANDLER(IDC_IGNORE_ADD, onIgnoreAdd)
		COMMAND_ID_HANDLER(IDC_IGNORE_REMOVE, onIgnoreRemove)
		COMMAND_ID_HANDLER(IDC_IGNORE_CLEAR, onIgnoreClear)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onFixControls(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onIgnoreAdd(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onIgnoreRemove(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);
	LRESULT onIgnoreClear(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */);

	void write();
	
private:
	typedef hash_set<string> StringHash;
	typedef StringHash::iterator StringHashIter;
	CComboBox timeCtrlBegin, timeCtrlEnd;
	static Item items[];
	static TextItem texts[];
	void fixControls();
	TCHAR* title;
	StringHash ignoreList;
	ExListViewCtrl ignoreListCtrl;

};

#endif // !defined(MISC_PAGE_H)
