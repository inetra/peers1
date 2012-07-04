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

#ifndef Sounds_H
#define Sounds_H

#include "PropPage.h"
#include "ExListViewCtrl.h"


class SoundsPage : 
  public PeersPropertyPage<IDD_SOUNDS>, 
  public PropPageImpl<SoundsPage,9>
{
public:
	SoundsPage() {
		title = _tcsdup((TSTRING(SETTINGS_APPEARANCE) + _T('\\') + TSTRING(SETTINGS_SOUNDS)).c_str());
		SetTitle(title);
	};

	~SoundsPage() {
		ctrlSounds.Detach();
		free(title);
	};

	BEGIN_MSG_MAP(SoundsPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		COMMAND_HANDLER(IDC_BROWSE, BN_CLICKED, onBrowse)
		COMMAND_HANDLER(IDC_PLAY, BN_CLICKED, onPlay)
		COMMAND_ID_HANDLER(IDC_NONE, onClickedNone)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT onBrowse(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onPlay(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
	LRESULT onClickedNone(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

	void write();

protected:
	static Item items[];
	static TextItem texts[];
	TCHAR* title;

	struct snds{
		ResourceManager::Strings name;
		int setting;
		string value;
	};

	static snds sounds[];

	ExListViewCtrl ctrlSounds;
};

#endif //Sounds_H

/**
 * @file
 * $Id: Sounds.h,v 1.4 2007/12/08 14:59:18 alexey Exp $
 */

