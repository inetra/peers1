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
#include "stdafx.h"
#include "FakeDetect.h"

PropPage::TextItem FakeDetect::texts[] = {
	{ DAA, ResourceManager::TEXT_FAKEPERCENT },
	{ IDC_TIMEOUTS, ResourceManager::ACCEPTED_TIMEOUTS },
	{ IDC_DISCONNECTS, ResourceManager::ACCEPTED_DISCONNECTS },
	{ IDC_PROT_USERS, ResourceManager::PROT_USERS },
	{ IDC_PROT_DESC, ResourceManager::PROT_DESC },
	{ IDC_PROT_FAVS, ResourceManager::PROT_FAVS },
        // !SMT!-S
        { IDC_BAN_SLOTS, ResourceManager::BAN_SLOTS },
        { IDC_BAN_SHARE, ResourceManager::BAN_SHARE },
        { IDC_BAN_LIMIT, ResourceManager::BAN_LIMIT },
        { IDC_BAN_MSG, ResourceManager::BAN_MESSAGE },
        { IDC_BANMSG_PERIOD, ResourceManager::BANMSG_PERIOD },
        { IDC_BAN_STEALTH, ResourceManager::BAN_STEALTH },
        { IDC_BAN_FORCE_PM, ResourceManager::BAN_FORCE_PM },
 	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 

PropPage::Item FakeDetect::items[] = {
	{ IDC_PERCENT_FAKE_SHARE_TOLERATED, SettingsManager::PERCENT_FAKE_SHARE_TOLERATED, PropPage::T_INT }, 
	{ IDC_TIMEOUTS_NO, SettingsManager::ACCEPTED_TIMEOUTS, PropPage::T_INT }, 
	{ IDC_DISCONNECTS_NO, SettingsManager::ACCEPTED_DISCONNECTS, PropPage::T_INT },
	{ IDC_PROT_PATTERNS, SettingsManager::PROT_USERS, PropPage::T_STR }, 
	{ IDC_PROT_FAVS, SettingsManager::PROT_FAVS, PropPage::T_BOOL }, 
        // !SMT!-S
        { IDC_BAN_SLOTS_NO, SettingsManager::BAN_SLOTS, PropPage::T_INT },
        { IDC_BAN_SHARE_NO, SettingsManager::BAN_SHARE, PropPage::T_INT },
        { IDC_BAN_LIMIT_NO, SettingsManager::BAN_LIMIT, PropPage::T_INT },
        { IDC_BAN_MSG_STR, SettingsManager::BAN_MESSAGE, PropPage::T_STR },
        { IDC_BANMSG_PERIOD_NO, SettingsManager::BANMSG_PERIOD, PropPage::T_INT },
        { IDC_BAN_STEALTH, SettingsManager::BAN_STEALTH, PropPage::T_BOOL },
        { IDC_BAN_FORCE_PM, SettingsManager::BAN_FORCE_PM, PropPage::T_BOOL },
	{ 0, 0, PropPage::T_END }
};

FakeDetect::ListItem FakeDetect::listItems[] = {
	{ SettingsManager::CHECK_NEW_USERS, ResourceManager::CHECK_ON_CONNECT },
	{ SettingsManager::DISPLAY_CHEATS_IN_MAIN_CHAT, ResourceManager::SETTINGS_DISPLAY_CHEATS_IN_MAIN_CHAT },
	{ SettingsManager::SHOW_SHARE_CHECKED_USERS, ResourceManager::SETTINGS_ADVANCED_SHOW_SHARE_CHECKED_USERS },
	{ SettingsManager::IP_IN_CHAT, ResourceManager::IP_IN_CHAT },
	{ SettingsManager::COUNTRY_IN_CHAT, ResourceManager::COUNTRY_IN_CHAT },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT FakeDetect::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	PropPage::read(items);
	PropPage::readCheckBoxList(listItems, GetDlgItem(IDC_FAKE_BOOLEANS));
	CComboBox cRaw;

#define ADDSTRINGS \
	cRaw.AddString(_T("No action")); \
	cRaw.AddString(Text::toT(SETTING(RAW1_TEXT)).c_str()); \
	cRaw.AddString(Text::toT(SETTING(RAW2_TEXT)).c_str()); \
	cRaw.AddString(Text::toT(SETTING(RAW3_TEXT)).c_str()); \
	cRaw.AddString(Text::toT(SETTING(RAW4_TEXT)).c_str()); \
	cRaw.AddString(Text::toT(SETTING(RAW5_TEXT)).c_str());

	cRaw.Attach(GetDlgItem(IDC_DISCONNECT_RAW));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::DISCONNECT_RAW));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_TIMEOUT_RAW));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::TIMEOUT_RAW));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_FAKE_RAW));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::FAKESHARE_RAW));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_LISTLEN));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::LISTLEN_MISMATCH));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_FILELIST_TOO_SMALL));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::FILELIST_TOO_SMALL));
	cRaw.Detach();

	cRaw.Attach(GetDlgItem(IDC_FILELIST_UNAVAILABLE));
	ADDSTRINGS
	cRaw.SetCurSel(settings->get(SettingsManager::FILELIST_UNAVAILABLE));
	cRaw.Detach();


	PropPage::translate(texts);

		

	// Do specialized reading here
	
	return TRUE;
}

void FakeDetect::write() {
	PropPage::write(items);
	PropPage::writeCheckBoxList(listItems, GetDlgItem(IDC_FAKE_BOOLEANS));
	
	// Do specialized writing here
	// settings->set(XX, YY);
	CComboBox cRaw(GetDlgItem(IDC_DISCONNECT_RAW));
	SettingsManager::getInstance()->set(SettingsManager::DISCONNECT_RAW, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_TIMEOUT_RAW);
	SettingsManager::getInstance()->set(SettingsManager::TIMEOUT_RAW, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_FAKE_RAW);
	SettingsManager::getInstance()->set(SettingsManager::FAKESHARE_RAW, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_LISTLEN);
	SettingsManager::getInstance()->set(SettingsManager::LISTLEN_MISMATCH, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_FILELIST_TOO_SMALL);
	SettingsManager::getInstance()->set(SettingsManager::FILELIST_TOO_SMALL, cRaw.GetCurSel());

	cRaw = GetDlgItem(IDC_FILELIST_UNAVAILABLE);
	SettingsManager::getInstance()->set(SettingsManager::FILELIST_UNAVAILABLE, cRaw.GetCurSel());
}

/**
 * @file
 * $Id: FakeDetect.cpp,v 1.5 2008/03/10 06:34:23 alexey Exp $
 */

