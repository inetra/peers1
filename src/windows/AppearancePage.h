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

#if !defined(APPEARANCE_PAGE_H)
#define APPEARANCE_PAGE_H


#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class AppearancePage : 
  public PeersPropertyPage<IDD_APPEARANCEPAGE>, 
  public PropPageImpl<AppearancePage,4>
{
public:
	AppearancePage() {
		SetTitle(CTSTRING(SETTINGS_APPEARANCE));
	}

	~AppearancePage();

	BEGIN_MSG_MAP(AppearancePage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_ID_HANDLER(IDC_BROWSE, onBrowse)
		COMMAND_HANDLER(IDC_TIMESTAMP_HELP, BN_CLICKED, onClickedHelp)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedHelp(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */);

	void write();
	
protected:
	static Item items[];
	static TextItem texts[];
	static ListItem listItems[];
	static ListItem boldItems[];
};

#endif // !defined(APPEARANCE_PAGE_H)

/**
 * @file
 * $Id: AppearancePage.h,v 1.4 2007/12/08 14:59:18 alexey Exp $
 */
