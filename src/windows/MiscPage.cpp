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
#include "MiscPage.h"
#include "../peers/PeersVersion.h"
#include "../client/IgnoreManager.h"

PropPage::TextItem MiscPage::texts[] = {
	{ IDC_MISC_APPEARANCE, ResourceManager::SETTINGS_APPEARANCE },
	{ IDC_TIME_AWAY, ResourceManager::SET_SECONDARY_AWAY },
	{ IDC_ADV_MISC, ResourceManager::SETTINGS_ADVANCED3 },
	{ IDC_PSR_DELAY_STR, ResourceManager::PSR_DELAY },
	{ IDC_THOLD_STR, ResourceManager::THRESHOLD },
	{ IDC_BUFFERSIZE, ResourceManager::BUFFERSIZE },
	{ IDC_IGNORE_ADD, ResourceManager::ADD },
	{ IDC_IGNORE_REMOVE, ResourceManager::REMOVE },
	{ IDC_IGNORE_CLEAR, ResourceManager::IGNORE_CLEAR },
	{ IDC_MISC_IGNORE, ResourceManager::IGNORED_USERS },
	{ IDC_RAW_TEXTS, ResourceManager::RAW_TEXTS },
	{ IDC_BUFFER_STR2, ResourceManager::BUFFER_STR2 },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

PropPage::Item MiscPage::items[] = {
	{ IDC_SECONDARY_AWAY_MESSAGE, SettingsManager::SECONDARY_AWAY_MESSAGE, PropPage::T_STR },
	{ IDC_TIME_AWAY, SettingsManager::AWAY_TIME_THROTTLE, PropPage::T_BOOL },
	{ IDC_AWAY_START_TIME, SettingsManager::AWAY_START, PropPage::T_INT },
	{ IDC_AWAY_END_TIME, SettingsManager::AWAY_END, PropPage::T_INT }, 
	{ IDC_PSR_DELAY, SettingsManager::PSR_DELAY, PropPage::T_INT },
	{ IDC_THOLD, SettingsManager::USER_THERSHOLD, PropPage::T_INT },
	{ IDC_BUFFERSIZE, SettingsManager::CHATBUFFERSIZE, PropPage::T_INT },
	{ IDC_RAW1_TEXT, SettingsManager::RAW1_TEXT, PropPage::T_STR },
	{ IDC_RAW2_TEXT, SettingsManager::RAW2_TEXT, PropPage::T_STR },
	{ IDC_RAW3_TEXT, SettingsManager::RAW3_TEXT, PropPage::T_STR },
	{ IDC_RAW4_TEXT, SettingsManager::RAW4_TEXT, PropPage::T_STR },
	{ IDC_RAW5_TEXT, SettingsManager::RAW5_TEXT, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

LRESULT MiscPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate(texts);
	PropPage::read(items);

	timeCtrlBegin.Attach(GetDlgItem(IDC_AWAY_START_TIME));
	timeCtrlEnd.Attach(GetDlgItem(IDC_AWAY_END_TIME));

	timeCtrlBegin.AddString(CTSTRING(MIDNIGHT));
	timeCtrlEnd.AddString(CTSTRING(MIDNIGHT));
	for (int i = 1; i < 12; ++i)
	{
		timeCtrlBegin.AddString((Text::toT(Util::toString(i)) + CTSTRING(AM)).c_str());
		timeCtrlEnd.AddString((Text::toT(Util::toString(i)) + CTSTRING(AM)).c_str());
	}
	timeCtrlBegin.AddString(CTSTRING(NOON));
	timeCtrlEnd.AddString(CTSTRING(NOON));
	for (int i = 1; i < 12; ++i)
	{
		timeCtrlBegin.AddString((Text::toT(Util::toString(i)) + CTSTRING(PM)).c_str());
		timeCtrlEnd.AddString((Text::toT(Util::toString(i)) + CTSTRING(PM)).c_str());
	}

	timeCtrlBegin.SetCurSel(SETTING(AWAY_START));
	timeCtrlEnd.SetCurSel(SETTING(AWAY_END));

	timeCtrlBegin.Detach();
	timeCtrlEnd.Detach();

	CRect rc;
	ignoreList = IgnoreManager::getInstance()->getIgnoredUsers();

	ignoreListCtrl.Attach(GetDlgItem(IDC_IGNORELIST));
	ignoreListCtrl.GetClientRect(rc);
	ignoreListCtrl.InsertColumn(0, _T("Dummy"), LVCFMT_LEFT, (rc.Width() - 17), 0);
	ignoreListCtrl.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	for(StringHash::iterator i = ignoreList.begin(); i != ignoreList.end(); ++i) {
		ignoreListCtrl.insert(ignoreListCtrl.GetItemCount(), Text::toT(*i));
	}

	fixControls();
	return TRUE;
}

LRESULT MiscPage::onFixControls(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	fixControls();
	return 0;
}

LRESULT MiscPage::onIgnoreAdd(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	TCHAR buf[256];
	if(GetDlgItemText(IDC_IGNORELIST_EDIT, buf, 256)) {
		pair<StringHashIter, bool> p = ignoreList.insert(Text::fromT(buf));
	
		if(p.second) {
			ignoreListCtrl.insert(ignoreListCtrl.GetItemCount(), buf);
		} else {
			MessageBox(CTSTRING(ALREADY_IGNORED), _T(APPNAME) _T(" ") _T(VERSIONSTRING), MB_OK);
		}
	}

	SetDlgItemText(IDC_IGNORELIST_EDIT, _T(""));
	return 0;
}

LRESULT MiscPage::onIgnoreRemove(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	int i = -1;
	
	TCHAR buf[256];

	while((i = ignoreListCtrl.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ignoreListCtrl.GetItemText(i, 0, buf, 256);

		ignoreList.erase(Text::fromT(buf));
		ignoreListCtrl.DeleteItem(i);
	}
	return 0;
}

LRESULT MiscPage::onIgnoreClear(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	ignoreListCtrl.DeleteAllItems();
	ignoreList.clear();
	return 0;
}

void MiscPage::fixControls() {
	bool state = (IsDlgButtonChecked(IDC_TIME_AWAY) != 0);
	::EnableWindow(GetDlgItem(IDC_AWAY_START_TIME), state);
	::EnableWindow(GetDlgItem(IDC_AWAY_END_TIME), state);
	::EnableWindow(GetDlgItem(IDC_SECONDARY_AWAY_MESSAGE), state);
	::EnableWindow(GetDlgItem(IDC_SECONDARY_AWAY_MSG), state);
	::EnableWindow(GetDlgItem(IDC_AWAY_TO), state);
}

void MiscPage::write()
{
	PropPage::write(items);

	timeCtrlBegin.Attach(GetDlgItem(IDC_AWAY_START_TIME));
	timeCtrlEnd.Attach(GetDlgItem(IDC_AWAY_END_TIME));
	settings->set(SettingsManager::AWAY_START, timeCtrlBegin.GetCurSel());
	settings->set(SettingsManager::AWAY_END, timeCtrlEnd.GetCurSel());
	timeCtrlBegin.Detach();
	timeCtrlEnd.Detach();
	IgnoreManager::getInstance()->putIgnoredUsers(ignoreList);

	/*if(SETTING(PSR_DELAY) < 5)
		settings->set(SettingsManager::PSR_DELAY, 5);*/
}
