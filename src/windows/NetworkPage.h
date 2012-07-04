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

#if !defined(NETWORK_PAGE_H)
#define NETWORK_PAGE_H

#include <atlcrack.h>
#include "PropPage.h"
#include "../peers/HelpHyperLink.h"

class NetworkPage : 
  public PeersPropertyPage<IDD_NETWORKPAGE>, 
  public PropPageImpl<NetworkPage,1,false>
{
public:
  NetworkPage() {
    SetTitle(CTSTRING(SETTINGS_NETWORK));
  }
  ~NetworkPage() {
  }

  BEGIN_MSG_MAP(NetworkPage)
    MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
    MESSAGE_HANDLER(WM_SIZE, onSize)
    COMMAND_ID_HANDLER(IDC_DIRECT, onClickedActive)
    COMMAND_ID_HANDLER(IDC_FIREWALL_PASSIVE, onClickedActive)
    COMMAND_ID_HANDLER(IDC_FIREWALL_UPNP, onClickedActive)
    COMMAND_ID_HANDLER(IDC_FIREWALL_NAT, onClickedActive)
    COMMAND_ID_HANDLER(IDC_DIRECT_OUT, onClickedActive)
    COMMAND_ID_HANDLER(IDC_SOCKS5, onClickedActive)
  END_MSG_MAP()

  LRESULT onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

  virtual void write();
  virtual void onExpertModeToggle();
private:
  static Item items[];
  static TextItem texts[];
  CHelpHyperLink helpLink;
  //CHyperLink ConnCheckUrl;
  CComboBox BindCombo;
  void fixControls();
  void getAddresses();
  bool isSupportedUPnP() const;
};

#endif // !defined(NETWORK_PAGE_H)

/**
 * @file
 * $Id: NetworkPage.h,v 1.9 2007/12/08 14:59:18 alexey Exp $
 */
