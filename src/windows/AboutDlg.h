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

#if !defined(ABOUT_DLG_H)
#define ABOUT_DLG_H

#include <atlctrlx.h>
#include "../peers/PeersVersion.h"
#include "../peers/CaptionFont.h"

class AboutDlg : public CDialogImpl<AboutDlg> {

public:
  enum { IDD = IDD_ABOUTBOX };

  AboutDlg() { }
  virtual ~AboutDlg() { }

  BEGIN_MSG_MAP(AboutDlg)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
    COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
    COMMAND_ID_HANDLER(IDC_LINK, onLink)
    COMMAND_ID_HANDLER(IDC_SUPPORT_MAIL, onMailLink)
  END_MSG_MAP()

  LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onMailLink (WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
  LRESULT onLink (WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

  LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    EndDialog(wID);
    return 0;
  }

private:
  auto_ptr<CaptionFont> developedFont;
  CToolTipCtrl m_versionTip;
  CHyperLink developedURL;
  CHyperLink url;
  CHyperLink m_Mail;

  void initVersionTip();
  AboutDlg(const AboutDlg&) { dcassert(false); }
};

#endif // !defined(ABOUT_DLG_H)

/**
* @file
* $Id: AboutDlg.h,v 1.8 2008/03/10 07:42:28 alexey Exp $
*/
