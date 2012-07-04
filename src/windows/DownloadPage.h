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

#if !defined(DOWNLOAD_PAGE_H)
#define DOWNLOAD_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"

class DownloadPage : 
  public PeersPropertyPage<IDD_DOWNLOADPAGE>, 
  public PropPageImpl<DownloadPage,3,false>
{
public:
	DownloadPage() {
		SetTitle(CTSTRING(SETTINGS_DOWNLOADS));
	}
	~DownloadPage() { }

	BEGIN_MSG_MAP(DownloadPage)
		MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
		MESSAGE_HANDLER(WM_SIZE, onSize)
		COMMAND_ID_HANDLER(IDC_BROWSEDIR, onClickedBrowseDir)
		COMMAND_ID_HANDLER(IDC_BROWSETEMPDIR, onClickedBrowseTempDir)
	END_MSG_MAP()

	LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onClickedBrowseDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClickedBrowseTempDir(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void write();
        virtual void onExpertModeToggle();
	
protected:
	static Item items[];
	static TextItem texts[];
private:
  void browseDirectory(int controlId);
  void updateDirectoryControlsLayout();
};

#endif //  !defined(DOWNLOAD_PAGE_H)

/**
 * @file
 * $Id: DownloadPage.h,v 1.7.2.1 2008/12/04 04:13:25 alexey Exp $
 */
