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

#if !defined(LOG_PAGE_H)
#define LOG_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class LogPage : 
  public PeersPropertyPage<IDD_LOGPAGE>, 
  public PropPageImpl<LogPage,14>
{
public:
  LogPage(): oldSelection(-1) {
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_LOGS)).c_str());
		SetTitle(title);
	}

	~LogPage() { free(title); }

	BEGIN_MSG_MAP(LogPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_BROWSE_LOG, onClickedBrowseDir)
		NOTIFY_HANDLER(IDC_LOG_OPTIONS, LVN_ITEMCHANGED, onItemChanged)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

	void write();

protected:
	static Item items[];
	static TextItem texts[];
	static ListItem listItems[];
	TCHAR* title;

	ExListViewCtrl logOptions;
	
	int oldSelection;
	
	//store all log options here so we can discard them
	//if the user cancels the dialog.
	//.first is filename and .second is format
	TStringPairList options;

	void getValues();
};

#endif // !defined(LOG_PAGE_H)

/**
 * @file
 * $Id: LogPage.h,v 1.4 2007/12/08 14:59:18 alexey Exp $
 */
