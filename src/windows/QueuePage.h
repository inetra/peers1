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

#if !defined(QUEUE_PAGE_H)
#define QUEUE_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"

class QueuePage : 
  public PeersPropertyPage<IDD_QUEUEPAGE>, 
  public PropPageImpl<QueuePage,18>
{
public:
	QueuePage() {
		title = _tcsdup((TSTRING(SETTINGS_DOWNLOADS) + _T('\\') + TSTRING(SETTINGS_QUEUE)).c_str());
		SetTitle(title);
	}
	~QueuePage() { free(title);	}

	BEGIN_MSG_MAP(QueuePage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	void write();
	
protected:

	static Item items[];
	static TextItem texts[];
	static ListItem optionItems[];
	TCHAR* title;

	CComboBox ctrlMultiSource;
};

#endif // !defined(QUEUE_PAGE_H)

/**
 * @file
 * $Id: QueuePage.h,v 1.4 2007/12/08 14:59:18 alexey Exp $
 */
