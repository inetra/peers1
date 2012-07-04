/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#ifndef SDCPage_H
#define SDCPage_H

#include "PropPage.h"


class SDCPage : 
  public PeersPropertyPage<IDD_SDCPAGE>, 
  public PropPageImpl<SDCPage,13>
{
public:
	SDCPage() { 
		title = _tcsdup((TSTRING(SETTINGS_ADVANCED) + _T('\\') + TSTRING(SETTINGS_ADVANCED3)).c_str());
		SetTitle(title);
	};

	virtual ~SDCPage() {
		ctrlShutdownAction.Detach();
		free(title);
	};

	BEGIN_MSG_MAP(SDCPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	
	virtual void write();

protected:
	static Item items[];
	static TextItem texts[];

	CComboBox ctrlShutdownAction;
	CComboBox userlistaction, transferlistaction, chataction
		, favuserlistaction; // !SMT!-UI
	CComboBox userlistChatAction;

	TCHAR* title;
};

#endif //SDCPage_H

/**
 * @file
 * $Id: SDCPage.h,v 1.4 2007/12/08 14:59:18 alexey Exp $
 */

