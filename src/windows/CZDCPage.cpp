/* 
 * 
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

#include "CZDCPage.h"

PropPage::TextItem CZDCPage::texts[] = {
	{ IDC_CZDC_WINAMP, ResourceManager::SETCZDC_WINAMP },

	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
}; 

CZDCPage* current_page;

PropPage::Item CZDCPage::items[] = {
	{ IDC_WINAMP, SettingsManager::WINAMP_FORMAT, PropPage::T_STR },
	{ 0, 0, PropPage::T_END }
};

CZDCPage::ListItem CZDCPage::listItems[] = {
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};


LRESULT CZDCPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);
	PropPage::read((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));



	// Do specialized reading here
	
	return TRUE;
}

void CZDCPage::write()
{
	PropPage::write((HWND)*this, items, listItems, GetDlgItem(IDC_ADVANCED_BOOLEANS));

	// Do specialized writing here
	// settings->set(XX, YY);
}

LRESULT CZDCPage::onClickedWinampHelp(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	MessageBox(CTSTRING(WINAMP_HELP), CTSTRING(WINAMP_HELP_DESC), MB_OK | MB_ICONINFORMATION);
	return S_OK;
}
