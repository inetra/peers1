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
#include "AppearancePage.h"

PropPage::TextItem AppearancePage::texts[] = {
	{ IDC_SETTINGS_APPEARANCE_OPTIONS, ResourceManager::SETTINGS_OPTIONS },
	{ IDC_SETTINGS_BOLD_CONTENTS, ResourceManager::SETTINGS_BOLD_OPTIONS },
	{ IDC_SETTINGS_TIME_STAMPS_FORMAT, ResourceManager::SETTINGS_TIME_STAMPS_FORMAT },
	{ IDC_SETTINGS_LANGUAGE_FILE, ResourceManager::SETTINGS_LANGUAGE_FILE },
	{ IDC_BROWSE, ResourceManager::BROWSE_ACCEL },
	{ IDC_SETTINGS_REQUIRES_RESTART, ResourceManager::SETTINGS_REQUIRES_RESTART },
//[-]PPA	{ IDC_SETTINGS_GET_USER_COUNTRY, ResourceManager::SETTINGS_GET_USER_COUNTRY }, 
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item AppearancePage::items[] = {
	{ IDC_TIME_STAMPS_FORMAT, SettingsManager::TIME_STAMPS_FORMAT, PropPage::T_STR },
	{ IDC_LANGUAGE, SettingsManager::LANGUAGE_FILE, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

PropPage::ListItem AppearancePage::listItems[] = {
	{ SettingsManager::SHOW_PROGRESS_BARS, ResourceManager::SETTINGS_SHOW_PROGRESS_BARS },
	{ SettingsManager::SHOW_INFOTIPS, ResourceManager::SETTINGS_SHOW_INFO_TIPS },
        { SettingsManager::FILTER_MESSAGES, ResourceManager::SETTINGS_FILTER_MESSAGES },
	{ SettingsManager::MINIMIZE_TRAY, ResourceManager::SETTINGS_MINIMIZE_TRAY },
	{ SettingsManager::EXPAND_QUEUE , ResourceManager::SETTINGS_EXPAND_QUEUE },
	{ SettingsManager::SORT_FAVUSERS_FIRST, ResourceManager::SETTINGS_SORT_FAVUSERS_FIRST },
	{ SettingsManager::USE_SYSTEM_ICONS, ResourceManager::SETTINGS_USE_SYSTEM_ICONS },
        { SettingsManager::GET_USER_COUNTRY, ResourceManager::SETTINGS_GET_USER_COUNTRY },
	{ SettingsManager::USE_OLD_SHARING_UI, ResourceManager::SETTINGS_USE_OLD_SHARING_UI },
	{ SettingsManager::USE_VERTICAL_VIEW, ResourceManager::SETTINGS_USE_VERTICAL_VIEW },
//+Added by Drakon	
	{ SettingsManager::UP_TRANSFER_COLORS, ResourceManager::UP_TRANSFER_COLORS }, 
//~Added by Drakon	
	{ SettingsManager::OLD_TRAY_BEHAV, ResourceManager::OLD_TRAY_BEHAV },
	{ SettingsManager::NON_HUBS_FRONT, ResourceManager::NON_HUBS_FRONT },
	{ SettingsManager::BLEND_OFFLINE_SEARCH, ResourceManager::BLEND_OFFLINE_SEARCH },
	{ SettingsManager::USE_CUSTOM_LIST_BACKGROUND, ResourceManager::USE_CUSTOM_LIST_BACKGROUND },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::ListItem AppearancePage::boldItems[] = {
	{ SettingsManager::BOLD_FINISHED_DOWNLOADS, ResourceManager::FINISHED_DOWNLOADS },
	{ SettingsManager::BOLD_FINISHED_UPLOADS, ResourceManager::FINISHED_UPLOADS },
	{ SettingsManager::BOLD_QUEUE, ResourceManager::DOWNLOAD_QUEUE },
	{ SettingsManager::BOLD_HUB, ResourceManager::HUB },
	{ SettingsManager::BOLD_PM, ResourceManager::PRIVATE_MESSAGE },
	{ SettingsManager::BOLD_SEARCH, ResourceManager::SEARCH },
	{ SettingsManager::BOLD_WAITING_USERS, ResourceManager::WAITING_USERS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

AppearancePage::~AppearancePage(){ }

void AppearancePage::write()
{
	PropPage::write(items);
	PropPage::writeCheckBoxList(listItems, GetDlgItem(IDC_APPEARANCE_BOOLEANS));
	PropPage::writeCheckBoxList(boldItems, GetDlgItem(IDC_BOLD_BOOLEANS));
}

LRESULT AppearancePage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate(texts);

	PropPage::read(items);
	PropPage::readCheckBoxList(listItems, GetDlgItem(IDC_APPEARANCE_BOOLEANS));
	PropPage::readCheckBoxList(boldItems, GetDlgItem(IDC_BOLD_BOOLEANS));
	
	SetDlgItemText(IDC_LANGUAGE, _T("Russian.xml")); //[+]PPA
	::EnableWindow(GetDlgItem(IDC_LANGUAGE), false); //[+]PPA
	::EnableWindow(GetDlgItem(IDC_BROWSE), false); //[+]PPA
	

	// Do specialized reading here
	return TRUE;
}

LRESULT AppearancePage::onBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
/*//[-]PPA
	TCHAR buf[MAX_PATH];
	static const TCHAR types[] = _T("Language Files\0*.xml\0All Files\0*.*\0");

	GetDlgItemText(IDC_LANGUAGE, buf, MAX_PATH);
	tstring x = buf;

	if(WinUtil::browseFile(x, m_hWnd, false, Text::toT(Util::getDataPath()), types) == IDOK) {
		SetDlgItemText(IDC_LANGUAGE, x.c_str());
	}
*/
	return 0;

}

LRESULT AppearancePage::onClickedHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	MessageBox(CTSTRING(TIMESTAMP_HELP), CTSTRING(TIMESTAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}

/**
 * @file
 * $Id: AppearancePage.cpp,v 1.7 2008/03/10 06:34:23 alexey Exp $
 */
