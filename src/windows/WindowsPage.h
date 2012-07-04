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

#if !defined(WINDOWS_PAGE_H)
#define WINDOWS_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"

class WindowsPage : 
  public PeersPropertyPage<IDD_WINDOWSPAGE>, 
  public PropPageImpl<WindowsPage,11>
{
public:
	WindowsPage() { 
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_WINDOWS)).c_str());
		SetTitle(title);
	}

	virtual ~WindowsPage() { free(title); }

	BEGIN_MSG_MAP(WindowsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	virtual void write();
	
protected:

	static TextItem textItem[];
	static ListItem listItems[];
	static ListItem optionItems[];
	static ListItem confirmItems[];

	TCHAR* title;
};

#endif // !defined(WINDOWS_PAGE_H)

/**
 * @file
 * $Id: WindowsPage.h,v 1.5 2007/12/08 14:59:18 alexey Exp $
 */
