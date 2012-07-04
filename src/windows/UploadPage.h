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

#if !defined(UPLOAD_PAGE_H)
#define UPLOAD_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "ExListViewCtrl.h"
#include "FolderTree.h"
#include "../peers/HelpHyperLink.h"

class UploadPage : 
  public PeersPropertyPage<IDD_UPLOADPAGE>, 
  public PropPageImpl<UploadPage,2,false>
{
public:
  UploadPage() {
    SetTitle(CTSTRING(SETTINGS_UPLOADS));
  }

  BEGIN_MSG_MAP(UploadPage)
    MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
    MESSAGE_HANDLER(WM_SIZE, onSize)
    MESSAGE_HANDLER(WM_DROPFILES, onDropFiles)
    MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
    REFLECT_NOTIFICATIONS()
  END_MSG_MAP()

  LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onDropFiles(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  virtual void write();
  virtual void onExpertModeToggle();

protected:
  static Item items[];
  static TextItem texts[];
  CStatic ctrlTotal;
  CFont totalFont;
  CFont smallFont;
  CHelpHyperLink helpLink;

  void addDirectory(const tstring& aPath);
  FolderTree ft;
  void initFonts();
};

#endif // !defined(UPLOAD_PAGE_H)

/**
 * @file
 * $Id: UploadPage.h,v 1.10 2008/03/10 07:51:06 alexey Exp $
 */
