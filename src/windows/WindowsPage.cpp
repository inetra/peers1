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

#include "WindowsPage.h"

#include "../client/FavoriteManager.h"

PropPage::TextItem WindowsPage::textItem[] = {
	{ IDC_SETTINGS_AUTO_OPEN, ResourceManager::SETTINGS_AUTO_OPEN },
	{ IDC_SETTINGS_WINDOWS_OPTIONS, ResourceManager::SETTINGS_WINDOWS_OPTIONS },
	{ IDC_SETTINGS_CONFIRM_OPTIONS, ResourceManager::SETTINGS_CONFIRM_DIALOG_OPTIONS },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

WindowsPage::ListItem WindowsPage::listItems[] = {
	{ SettingsManager::OPEN_ADVICE, ResourceManager::MENU_ADVICE_WINDOW },
	{ SettingsManager::OPEN_SUBSCRIPTIONS, ResourceManager::MENU_SUBSCRIPTION_WINDOW },
	{ SettingsManager::OPEN_FAVORITE_HUBS, ResourceManager::FAVORITE_HUBS },
	{ SettingsManager::OPEN_FAVORITE_USERS, ResourceManager::FAVORITE_USERS },
	{ SettingsManager::OPEN_QUEUE, ResourceManager::DOWNLOAD_QUEUE },
	{ SettingsManager::OPEN_FINISHED_DOWNLOADS, ResourceManager::FINISHED_DOWNLOADS },
	{ SettingsManager::OPEN_WAITING_USERS, ResourceManager::WAITING_USERS },
	{ SettingsManager::OPEN_FINISHED_UPLOADS, ResourceManager::FINISHED_UPLOADS },
	{ SettingsManager::OPEN_SEARCH_SPY, ResourceManager::SEARCH_SPY },
	{ SettingsManager::OPEN_NETWORK_STATISTICS, ResourceManager::NETWORK_STATISTICS },
	{ SettingsManager::OPEN_NOTEPAD, ResourceManager::NOTEPAD },
	{ SettingsManager::OPEN_CDMDEBUG, ResourceManager::MENU_CDMDEBUG_MESSAGES },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

WindowsPage::ListItem WindowsPage::optionItems[] = {
	{ SettingsManager::POPUP_PMS, ResourceManager::SETTINGS_POPUP_PMS },
	{ SettingsManager::POPUP_HUB_PMS, ResourceManager::SETTINGS_POPUP_HUB_PMS },
	{ SettingsManager::POPUP_BOT_PMS, ResourceManager::SETTINGS_POPUP_BOT_PMS },
	{ SettingsManager::POPUNDER_FILELIST, ResourceManager::SETTINGS_POPUNDER_FILELIST },
	{ SettingsManager::POPUNDER_PM, ResourceManager::SETTINGS_POPUNDER_PM },
	{ SettingsManager::JOIN_OPEN_NEW_WINDOW, ResourceManager::SETTINGS_OPEN_NEW_WINDOW },
	{ SettingsManager::IGNORE_HUB_PMS, ResourceManager::SETTINGS_IGNORE_HUB_PMS },
	{ SettingsManager::IGNORE_BOT_PMS, ResourceManager::SETTINGS_IGNORE_BOT_PMS },
	{ SettingsManager::UNUSED_TOGGLE_ACTIVE_WINDOW, ResourceManager::SETTINGS_TOGGLE_ACTIVE_WINDOW },
	{ SettingsManager::PROMPT_PASSWORD, ResourceManager::SETTINGS_PROMPT_PASSWORD },
	{ SettingsManager::MINIMIZE_ON_CLOSE, ResourceManager::MINIMIZE_ON_CLOSE },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

WindowsPage::ListItem WindowsPage::confirmItems[] = {
	{ SettingsManager::CONFIRM_EXIT, ResourceManager::SETTINGS_CONFIRM_EXIT },
	{ SettingsManager::CONFIRM_HUB_REMOVAL, ResourceManager::SETTINGS_CONFIRM_HUB_REMOVAL },
	{ SettingsManager::CONFIRM_DELETE, ResourceManager::SETTINGS_CONFIRM_ITEM_REMOVAL },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT WindowsPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate(textItem);
	PropPage::readCheckBoxList(listItems, GetDlgItem(IDC_WINDOWS_STARTUP));
	PropPage::readCheckBoxList(optionItems, GetDlgItem(IDC_WINDOWS_OPTIONS));
	PropPage::readCheckBoxList(confirmItems, GetDlgItem(IDC_CONFIRM_OPTIONS));

	// Do specialized reading here
	return TRUE;
}

void WindowsPage::write() {
	PropPage::writeCheckBoxList(listItems, GetDlgItem(IDC_WINDOWS_STARTUP));
	PropPage::writeCheckBoxList(optionItems, GetDlgItem(IDC_WINDOWS_OPTIONS));
	PropPage::writeCheckBoxList(confirmItems, GetDlgItem(IDC_CONFIRM_OPTIONS));
}

/**
 * @file
 * $Id: WindowsPage.cpp,v 1.7.2.2 2008/12/04 20:19:40 alexey Exp $
 */
