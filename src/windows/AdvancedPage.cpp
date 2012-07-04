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

#include "AdvancedPage.h"
#include "CommandDlg.h"

PropPage::TextItem AdvancedPage::texts[] = {
  { IDC_CZDC_WINAMP, ResourceManager::SETCZDC_WINAMP },
  { IDC_SKIPLIST_GP, ResourceManager::SETTINGS_SKIPLIST_SHARE },
  //[+] WhiteD. Custom ratio message
  { IDC_CZDC_RATIOMSG, ResourceManager::CZDC_RATIOMSG},
  // End of Addition.
  { 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AdvancedPage::items[] = {
  { IDC_SKIPLIST_SHARE, SettingsManager::SKIPLIST_SHARE, PropPage::T_STR },
  { IDC_EWMAGNET_TEMPL, SettingsManager::COPY_WMLINK, PropPage::T_STR},
  //[+] WhiteD. Custom ratio message
  { IDC_RATIOMSG, SettingsManager::RATIO_TEMPLATE, PropPage::T_STR},
  // End of Addition.
  { 0, 0, PropPage::T_END }
};

AdvancedPage::ListItem AdvancedPage::listItems[] = {
  { SettingsManager::AUTO_AWAY, ResourceManager::SETTINGS_AUTO_AWAY },
  { SettingsManager::AUTO_FOLLOW, ResourceManager::SETTINGS_AUTO_FOLLOW },
  { SettingsManager::MINIMIZE_ON_STARTUP, ResourceManager::SETTINGS_MINIMIZE_ON_STARTUP },
  //[+]Drakon
  //{ SettingsManager::STARTUP_BACKUP, ResourceManager::STARTUP_BACKUP },
  //[~]Drakon
  //{ SettingsManager::USE_EXTENSION_DOWNTO, ResourceManager::SETTINGS_USE_EXTENSION_DOWNTO },
  { SettingsManager::REMOVE_FORBIDDEN, ResourceManager::SETCZDC_REMOVE_FORBIDDEN },
  { SettingsManager::URL_HANDLER, ResourceManager::SETTINGS_URL_HANDLER },
  { SettingsManager::MAGNET_REGISTER, ResourceManager::SETCZDC_MAGNET_URI_HANDLER },
  { SettingsManager::KEEP_LISTS, ResourceManager::SETTINGS_KEEP_LISTS },
  { SettingsManager::AUTO_KICK, ResourceManager::SETTINGS_AUTO_KICK },
  { SettingsManager::AUTO_KICK_NO_FAVS, ResourceManager::SETTINGS_AUTO_KICK_NO_FAVS },
  { SettingsManager::ADLS_BREAK_ON_FIRST, ResourceManager::SETTINGS_ADLS_BREAK_ON_FIRST },
  { SettingsManager::COMPRESS_TRANSFERS, ResourceManager::SETTINGS_COMPRESS_TRANSFERS },
  { SettingsManager::HUB_USER_COMMANDS, ResourceManager::SETTINGS_HUB_USER_COMMANDS },
  //{ SettingsManager::SEARCH_PASSIVE, ResourceManager::SETCZDC_PASSIVE_SEARCH },
  { SettingsManager::SEND_UNKNOWN_COMMANDS, ResourceManager::SETTINGS_SEND_UNKNOWN_COMMANDS },
  //{ SettingsManager::ADD_FINISHED_INSTANTLY, ResourceManager::ADD_FINISHED_INSTANTLY },
  { SettingsManager::DEBUG_COMMANDS, ResourceManager::SETTINGS_DEBUG_COMMANDS },
  { SettingsManager::GARBAGE_COMMAND_INCOMING, ResourceManager::GARBAGE_INCOMING },
  { SettingsManager::GARBAGE_COMMAND_OUTGOING, ResourceManager::GARBAGE_OUTGOING },
  { SettingsManager::WEBSERVER, ResourceManager::SETTINGS_WEBSERVER }, 
  { SettingsManager::STRIP_TOPIC, ResourceManager::SETTINGS_STRIP_TOPIC },
  //{ SettingsManager::FILTER_ENTER, ResourceManager::SETTINGS_FILTER_ENTER },
  { SettingsManager::SHOW_SHELL_MENU, ResourceManager::SETTINGS_SHOW_SHELL_MENU },
  { SettingsManager::SETTINGS_STATE, ResourceManager::SETTINGS_STATE },
  { SettingsManager::ENABLE_HUBTOPIC, ResourceManager::ENABLE_HUBTOPIC },
  //{ SettingsManager::HUB_SMALL, ResourceManager::HUB_SMALL },
  { SettingsManager::OPEN_LOGS_INTERNAL, ResourceManager::OPEN_LOGS_INTERNAL },
  { SettingsManager::EXTERNAL_PREVIEW, ResourceManager::EXTERNAL_PREVIEW },
  { SettingsManager::OPEN_HUB_CHAT_ON_CONNECT, ResourceManager::OPEN_HUB_CHAT_ON_CONNECT },
  { 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT AdvancedPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  PropPage::translate(texts);
  PropPage::read(items);
  PropPage::readCheckBoxList(listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

  CurSel = SETTING(MEDIA_PLAYER);
  WMPlayerStr = Text::toT(SETTING(WMP_FORMAT));
  WinampStr = Text::toT(SETTING(WINAMP_FORMAT));
  iTunesStr = Text::toT(SETTING(ITUNES_FORMAT));
  MPCStr = Text::toT(SETTING(MPLAYERC_FORMAT));

  ctrlPlayer.Attach(GetDlgItem(IDC_PLAYER_COMBO));
  ctrlPlayer.AddString(_T("Winamp"));
  ctrlPlayer.AddString(_T("Windows Media Player"));
  ctrlPlayer.AddString(_T("iTunes"));
  ctrlPlayer.AddString(_T("Media Player Classic"));
  ctrlPlayer.SetCurSel(CurSel);

  loadFormat();
  return TRUE;
}

void AdvancedPage::write() {
  PropPage::write(items);
  PropPage::writeCheckBoxList(listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));
  saveFormat();
  settings->set(SettingsManager::MEDIA_PLAYER, ctrlPlayer.GetCurSel());
  settings->set(SettingsManager::WINAMP_FORMAT, Text::fromT(WinampStr).c_str());
  settings->set(SettingsManager::WMP_FORMAT, Text::fromT(WMPlayerStr).c_str());
  settings->set(SettingsManager::ITUNES_FORMAT, Text::fromT(iTunesStr).c_str());
  settings->set(SettingsManager::MPLAYERC_FORMAT, Text::fromT(MPCStr).c_str());
}

LRESULT AdvancedPage::onClickedWinampHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
  switch (CurSel)
  {
  case 0: MessageBox(CTSTRING(WINAMP_HELP), CTSTRING(WINAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION); break;
  case 1: MessageBox(CTSTRING(WMP_HELP), CTSTRING(WMP_HELP_DESC), MB_OK | MB_ICONINFORMATION); break;
  case 2: MessageBox(CTSTRING(ITUNES_HELP), CTSTRING(ITUNES_HELP_DESC), MB_OK | MB_ICONINFORMATION); break;
  case 3: MessageBox(CTSTRING(MPC_HELP), CTSTRING(MPC_HELP_DESC), MB_OK | MB_ICONINFORMATION); break;
  }
  return 0;
}

//[+] WhiteD. Custom ratio message
LRESULT AdvancedPage::onClickedRatioMsgHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
  MessageBox(CTSTRING(RATIO_MSG_HELP), CTSTRING(RATIO_MSG_HELP_DESC), MB_OK | MB_ICONINFORMATION);
  return 0;
}
// End of Addition.

void AdvancedPage::loadFormat() {
  bool enable;
  switch (CurSel) 
  {
  case 0: SetDlgItemText(IDC_WINAMP, WinampStr.c_str()); enable = true; break;
  case 1: SetDlgItemText(IDC_WINAMP, WMPlayerStr.c_str()); enable = true; break;
  case 2: SetDlgItemText(IDC_WINAMP, iTunesStr.c_str()); enable = true; break;
  case 3: SetDlgItemText(IDC_WINAMP, MPCStr.c_str()); enable = true; break;
  default:
    SetDlgItemText(IDC_WINAMP, CTSTRING(NO_MEDIA_SPAM));
    enable = false; 
    break;
  }
  GetDlgItem(IDC_WINAMP).EnableWindow(enable);
  GetDlgItem(IDC_WINAMP_HELP).EnableWindow(enable);
}

void AdvancedPage::saveFormat() {
  switch (CurSel)
  {
  case 0: WinampStr = GetDlgItemString(IDC_WINAMP); break;
  case 1: WMPlayerStr = GetDlgItemString(IDC_WINAMP); break;
  case 2: iTunesStr = GetDlgItemString(IDC_WINAMP); break;
  case 3: MPCStr = GetDlgItemString(IDC_WINAMP); break;
  }
}

LRESULT AdvancedPage::onSelChange(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
  saveFormat();
  CurSel = ctrlPlayer.GetCurSel();
  loadFormat();
  return 0;
}

/**
* @file
* $Id: AdvancedPage.cpp,v 1.9 2008/03/27 01:56:40 alexey Exp $
*/
