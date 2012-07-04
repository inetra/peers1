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

#include "stdafx.h"

#include "NetworkPage.h"

#include <IPHlpApi.h>
#include "../peers/PeersVersion.h"
#include "../peers/ControlAdjuster.h"
#pragma comment(lib, "iphlpapi.lib")

PropPage::TextItem NetworkPage::texts[] = {
	{ IDC_DIRECT, ResourceManager::SETTINGS_DIRECT },
	{ IDC_DIRECT_OUT, ResourceManager::SETTINGS_DIRECT },
	{ IDC_FIREWALL_UPNP, ResourceManager::SETTINGS_FIREWALL_UPNP },
	{ IDC_FIREWALL_NAT, ResourceManager::SETTINGS_FIREWALL_NAT },
	{ IDC_FIREWALL_PASSIVE, ResourceManager::SETTINGS_FIREWALL_PASSIVE },
	{ IDC_OVERRIDE, ResourceManager::SETTINGS_OVERRIDE },
	{ IDC_SOCKS5, ResourceManager::SETTINGS_SOCKS5 }, 
	{ IDC_SETTINGS_PORTS, ResourceManager::SETTINGS_PORTS },
	{ IDC_SETTINGS_IP, ResourceManager::SETTINGS_EXTERNAL_IP },
	{ IDC_SETTINGS_PORT_TCP, ResourceManager::SETTINGS_TCP_PORT },
	{ IDC_SETTINGS_PORT_UDP, ResourceManager::SETTINGS_UDP_PORT },
	{ IDC_SETTINGS_PORT_TLS, ResourceManager::SETTINGS_TLS_PORT },
	{ IDC_SETTINGS_SOCKS5_IP, ResourceManager::SETTINGS_SOCKS5_IP },
	{ IDC_SETTINGS_SOCKS5_PORT, ResourceManager::SETTINGS_SOCKS5_PORT },
	{ IDC_SETTINGS_SOCKS5_USERNAME, ResourceManager::SETTINGS_SOCKS5_USERNAME },
	{ IDC_SETTINGS_SOCKS5_PASSWORD, ResourceManager::PASSWORD },
	{ IDC_SOCKS_RESOLVE, ResourceManager::SETTINGS_SOCKS5_RESOLVE },
	{ IDC_SETTINGS_INCOMING, ResourceManager::SETTINGS_INCOMING },
	{ IDC_SETTINGS_OUTGOING, ResourceManager::SETTINGS_OUTGOING },
	{ IDC_SETTINGS_BIND_ADDRESS, ResourceManager::SETTINGS_BIND_ADDRESS },
	{ IDC_CON_CHECK, ResourceManager::CHECK_SETTINGS },
	{ IDC_IPUPDATE, ResourceManager::UPDATE_IP },

	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item NetworkPage::items[] = {
	{ IDC_PORT_TCP,		SettingsManager::TCP_PORT,		PropPage::T_INT }, 
	{ IDC_PORT_UDP,		SettingsManager::UDP_PORT,		PropPage::T_INT }, 
	{ 0, 0, PropPage::T_END }
};

void NetworkPage::write()
{
	PropPage::write(items);
}

bool NetworkPage::isSupportedUPnP() const {
#ifdef PPA_INCLUDE_UPNP
  //WinXP & WinSvr2003, Vista
  return (WinUtil::getOsMajor() >= 5 && WinUtil::getOsMinor() >= 1) || (WinUtil::getOsMajor() >= 6);
#else
  return false;
#endif		
}

LRESULT NetworkPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  PropPage::translate(texts);
  initHeader(IDC_SETTINGS_INCOMING);
  helpLink.SetHyperLink((tstring(HELP_REDIRECT_URL) + _T("network")).c_str());
  helpLink.Create(m_hWnd, NULL, CTSTRING(SETTINGS_NETWORK_HELP_LINK));

  if (!isSupportedUPnP()) {
    ::EnableWindow(GetDlgItem(IDC_FIREWALL_UPNP), FALSE);
  }

  switch(SETTING(INCOMING_CONNECTIONS)) 
  {
  case SettingsManager::INCOMING_DIRECT: 
  case SettingsManager::INCOMING_FIREWALL_UPNP: 
  case SettingsManager::INCOMING_FIREWALL_NAT: 
    CheckDlgButton(IDC_DIRECT, BST_CHECKED); 
    break;
  default: 
    CheckDlgButton(IDC_FIREWALL_PASSIVE, BST_CHECKED); 
    break;
  }

  PropPage::read(items);

  fixControls();

  return TRUE;
}

LRESULT NetworkPage::onSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  if (helpLink) {
    CRect r;
    GetClientRect(r);
    int helpW, helpH;
    helpLink.GetIdealSize(helpW, helpH, r.Width() / 2);
    CRect headerRect;
    GetDlgItem(IDC_SETTINGS_INCOMING).GetWindowRect(headerRect);
    ScreenToClient(headerRect);
    helpLink.MoveWindow(r.right - helpW, max(0, (int) (headerRect.top + headerRect.bottom - helpH) / 2), helpW, helpH);
  }
  return 0;
}

void NetworkPage::onExpertModeToggle() {
  static const int expertModeControlIds[] = {
    IDC_SETTINGS_PORTS,
	IDC_PORT_TCP,
	IDC_PORT_UDP,
    IDC_SETTINGS_PORT_TCP,
    IDC_SETTINGS_PORT_UDP
  };
  ShowWindows(expertModeControlIds, isExpertMode());
}

void NetworkPage::fixControls() {
  //	BOOL direct = IsDlgButtonChecked(IDC_DIRECT) == BST_CHECKED;
  static const int firewallControlIds[] = { 
    IDC_DIRECT, 
    IDC_FIREWALL_PASSIVE
  };
  //const bool firewall = IsDlgButtonChecked(IDC_FIREWALL_UPNP) == BST_CHECKED || IsDlgButtonChecked(IDC_FIREWALL_NAT) == BST_CHECKED;
  EnableWindows(firewallControlIds, false);
}

void NetworkPage::getAddresses() {
	IP_ADAPTER_INFO* AdapterInfo = NULL;
	DWORD dwBufLen = NULL;

	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	if(dwStatus == ERROR_BUFFER_OVERFLOW) {
		AdapterInfo = (IP_ADAPTER_INFO*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBufLen);
		dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);		
	}

	if(dwStatus == ERROR_SUCCESS) {
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
		while (pAdapterInfo) {
			IP_ADDR_STRING* pIpList = &(pAdapterInfo->IpAddressList);
			while (pIpList) {
				BindCombo.AddString(Text::toT(pIpList->IpAddress.String).c_str());
				pIpList = pIpList->Next;
			}
			pAdapterInfo = pAdapterInfo->Next;
		}
	}
//+BugMaster: memory leak
	if(AdapterInfo)
		HeapFree(GetProcessHeap(), 0, AdapterInfo);
//-BugMaster: memory leak

}

LRESULT NetworkPage::onClickedActive(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}
