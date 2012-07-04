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

#if !defined(ADVANCED_PAGE_H)
#define ADVANCED_PAGE_H

#include "PropPage.h"

class AdvancedPage : 
  public PeersPropertyPage<IDD_ADVANCEDPAGE>, 
  public PropPageImpl<AdvancedPage,12>
{
public:
  AdvancedPage() {
    SetTitle(CTSTRING(SETTINGS_ADVANCED));
  }

  ~AdvancedPage() { }

  BEGIN_MSG_MAP(AdvancedPage)
    MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
    COMMAND_HANDLER(IDC_WINAMP_HELP, BN_CLICKED, onClickedWinampHelp)
    COMMAND_HANDLER(IDC_PLAYER_COMBO, CBN_SELCHANGE, onSelChange)
    COMMAND_HANDLER(IDC_RATIOMSG_HELP, BN_CLICKED, onClickedRatioMsgHelp)
  END_MSG_MAP()

  LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onClickedWinampHelp(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */);
  //[+] WhiteD. Custom ratio message
  LRESULT onClickedRatioMsgHelp(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */);
  // End of Addition.
  LRESULT onSelChange(WORD /* wNotifyCode */, WORD wID, HWND /* hWndCtl */, BOOL& /* bHandled */);

  virtual void write();

protected:

  static Item items[];
  static TextItem texts[];
  static ListItem listItems[];

  CComboBox ctrlPlayer;

  tstring WinampStr;
  tstring WMPlayerStr;
  tstring iTunesStr;
  tstring MPCStr;
  int CurSel;
  void loadFormat();
  void saveFormat();
};

#endif // !defined(ADVANCED_PAGE_H)

/**
* @file
* $Id: AdvancedPage.h,v 1.5 2007/12/08 17:29:20 alexey Exp $
*/
