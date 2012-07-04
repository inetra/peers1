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
#include "FavHubProperties.h"
#include "../client/FavoriteManager.h"

LRESULT FavHubProperties::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
	// Translate dialog
	SetWindowText(CTSTRING(FAVORITE_HUB_PROPERTIES));
	SetDlgItemText(IDC_FH_HUB, CTSTRING(HUB));
	SetDlgItemText(IDC_FH_IDENT, CTSTRING(FAVORITE_HUB_IDENTITY));
	SetDlgItemText(IDC_FH_NAME, CTSTRING(HUB_NAME));
	SetDlgItemText(IDC_FH_ADDRESS, CTSTRING(HUB_ADDRESS));
	SetDlgItemText(IDC_FH_HUB_DESC, CTSTRING(DESCRIPTION));
	SetDlgItemText(IDC_FH_NICK, CTSTRING(NICK));
	SetDlgItemText(IDC_FH_PASSWORD, CTSTRING(PASSWORD));
	SetDlgItemText(IDC_FH_USER_DESC, CTSTRING(DESCRIPTION));
	SetDlgItemText(IDC_FH_EMAIL, CTSTRING(EMAIL));
	SetDlgItemText(IDC_DEFAULT, CTSTRING(DEFAULT));
	SetDlgItemText(IDC_ACTIVE, CTSTRING(SETTINGS_DIRECT));
	SetDlgItemText(IDC_PASSIVE, CTSTRING(SETTINGS_FIREWALL_PASSIVE));
	SetDlgItemText(IDC_HIDE_SHARE, CTSTRING(HIDE_SHARE));
	SetDlgItemText(IDC_SHOW_JOINS, CTSTRING(SHOW_JOINS));
	SetDlgItemText(IDC_EXCL_CHECKS, CTSTRING(EXCL_CHECKS));
	SetDlgItemText(IDC_STEALTH, CTSTRING(STEALTH_MODE));
	SetDlgItemText(IDC_RAW1, Text::toT(SETTING(RAW1_TEXT)).c_str());
	SetDlgItemText(IDC_RAW2, Text::toT(SETTING(RAW2_TEXT)).c_str());
	SetDlgItemText(IDC_RAW3, Text::toT(SETTING(RAW3_TEXT)).c_str());
	SetDlgItemText(IDC_RAW4, Text::toT(SETTING(RAW4_TEXT)).c_str());
	SetDlgItemText(IDC_RAW5, Text::toT(SETTING(RAW5_TEXT)).c_str());
	SetDlgItemText(IDC_OPCHAT, CTSTRING(OPCHAT));
	SetDlgItemText(IDC_CONN_BORDER, CTSTRING(CONNECTION));
        SetDlgItemText(IDC_CLIENT_ID, CTSTRING(CLIENT_ID)); // !SMT!-S

	// Fill in values
	SetDlgItemText(IDC_HUBNAME, Text::toT(entry->getName()).c_str());
	SetDlgItemText(IDC_HUBDESCR, Text::toT(entry->getDescription()).c_str());
	SetDlgItemText(IDC_HUBADDR, Text::toT(entry->getServer()).c_str());
	SetDlgItemText(IDC_HUBNICK, Text::toT(entry->getNick(false)).c_str());
	SetDlgItemText(IDC_HUBPASS, Text::toT(entry->getPassword()).c_str());
	SetDlgItemText(IDC_HUBUSERDESCR, Text::toT(entry->getUserDescription()).c_str());
	SetDlgItemText(IDC_HUBAWAY, Text::toT(entry->getAwayMsg()).c_str());
	SetDlgItemText(IDC_HUBEMAIL, Text::toT(entry->getEmail()).c_str());
	CheckDlgButton(IDC_STEALTH, entry->getStealth() ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_HIDE_SHARE, entry->getHideShare() ? BST_CHECKED : BST_UNCHECKED); // Hide Share Mod
	CheckDlgButton(IDC_SHOW_JOINS, entry->getShowJoins() ? BST_CHECKED : BST_UNCHECKED); // Show joins
	CheckDlgButton(IDC_EXCL_CHECKS, entry->getExclChecks() ? BST_CHECKED : BST_UNCHECKED); // Excl. from client checking
	SetDlgItemText(IDC_RAW_ONE, Text::toT(entry->getRawOne()).c_str());
	SetDlgItemText(IDC_RAW_TWO, Text::toT(entry->getRawTwo()).c_str());
	SetDlgItemText(IDC_RAW_THREE, Text::toT(entry->getRawThree()).c_str());
	SetDlgItemText(IDC_RAW_FOUR, Text::toT(entry->getRawFour()).c_str());
	SetDlgItemText(IDC_RAW_FIVE, Text::toT(entry->getRawFive()).c_str());
	SetDlgItemText(IDC_SERVER, Text::toT(entry->getIP()).c_str());
	SetDlgItemText(IDC_OPCHAT_STR, Text::toT(entry->getOpChat()).c_str());

        // !SMT!-S
        IdCombo.Attach(GetDlgItem(IDC_CLIENT_ID_BOX));
        IdCombo.AddString(_T("ApexDC++ V:0.2.2"));
        IdCombo.AddString(_T("ApexDC++ V:0.3.0"));
        IdCombo.AddString(_T("ApexDC++ V:0.4.0"));
        IdCombo.AddString(_T("ApexDC++ V:1.0.0B2"));
        IdCombo.AddString(_T("PWDC++ V:0.41"));
        IdCombo.AddString(_T("IceDC++ V:1.00a"));
        IdCombo.AddString(_T("StrgDC++ V:2.03"));
        IdCombo.AddString(_T("Olympus P2P v4.0 RC3"));
		
        SetDlgItemText(IDC_CLIENT_ID_BOX, Text::toT(entry->getClientId()).c_str());
        CheckDlgButton(IDC_CLIENT_ID, entry->getOverrideId() ? BST_CHECKED : BST_UNCHECKED);
        BOOL x; OnChangeId(BN_CLICKED,IDC_CLIENT,0,x);
        // end !SMT!-S

	if(entry->getMode() == 0)
		CheckRadioButton(IDC_ACTIVE, IDC_DEFAULT, IDC_DEFAULT);
	else if(entry->getMode() == 1)
		CheckRadioButton(IDC_ACTIVE, IDC_DEFAULT, IDC_ACTIVE);
	else if(entry->getMode() == 2)
		CheckRadioButton(IDC_ACTIVE, IDC_DEFAULT, IDC_PASSIVE);

	CEdit tmp;
	tmp.Attach(GetDlgItem(IDC_HUBNAME));
	tmp.SetFocus();
	tmp.SetSel(0,-1);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBNICK));
	tmp.LimitText(35);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBUSERDESCR));
	tmp.LimitText(50);
	tmp.Detach();
	tmp.Attach(GetDlgItem(IDC_HUBPASS));
	tmp.SetPasswordChar('*');
	tmp.Detach();
	CenterWindow(GetParent());

	const string& url = entry->getServer();
	if(Util::strnicmp("adc://", url.c_str(), 6) == 0 || Util::strnicmp("adcs://", url.c_str(), 7) == 0) {
		if(IsDlgButtonChecked(IDC_HIDE_SHARE) == 1)
			CheckDlgButton(IDC_HIDE_SHARE, BST_UNCHECKED);
		::EnableWindow(GetDlgItem(IDC_HIDE_SHARE), false);
	}

	return FALSE;
}

LRESULT FavHubProperties::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(wID == IDOK)
	{
		TCHAR buf[512];
		GetDlgItemText(IDC_HUBADDR, buf, 256);
		if(buf[0] == '\0') {
			MessageBox(CTSTRING(INCOMPLETE_FAV_HUB), _T(""), MB_ICONWARNING | MB_OK);
			return 0;
		}
		const string& url = Text::fromT(buf);
		entry->setServer(Text::fromT(buf));
		GetDlgItemText(IDC_HUBNAME, buf, 256);
		entry->setName(Text::fromT(buf));
		GetDlgItemText(IDC_HUBDESCR, buf, 256);
		entry->setDescription(Text::fromT(buf));
		GetDlgItemText(IDC_HUBNICK, buf, 256);
		entry->setNick(Text::fromT(buf));
		GetDlgItemText(IDC_HUBPASS, buf, 256);
		entry->setPassword(Text::fromT(buf));
		GetDlgItemText(IDC_HUBUSERDESCR, buf, 256);
		entry->setUserDescription(Text::fromT(buf));
		GetDlgItemText(IDC_HUBAWAY, buf, 256);
		entry->setAwayMsg(Text::fromT(buf));
		GetDlgItemText(IDC_HUBEMAIL, buf, 256);
		entry->setEmail(Text::fromT(buf));
		entry->setStealth(IsDlgButtonChecked(IDC_STEALTH) == 1);
		entry->setHideShare(IsDlgButtonChecked(IDC_HIDE_SHARE) == 1); // Hide Share Mod
		entry->setShowJoins(IsDlgButtonChecked(IDC_SHOW_JOINS) == 1); // Show joins
		entry->setExclChecks(IsDlgButtonChecked(IDC_EXCL_CHECKS) == 1); // Excl. from client checking
		GetDlgItemText(IDC_RAW_ONE, buf, 512);
		entry->setRawOne(Text::fromT(buf));
		GetDlgItemText(IDC_RAW_TWO, buf, 512);
		entry->setRawTwo(Text::fromT(buf));
		GetDlgItemText(IDC_RAW_THREE, buf, 512);
		entry->setRawThree(Text::fromT(buf));
		GetDlgItemText(IDC_RAW_FOUR, buf, 512);
		entry->setRawFour(Text::fromT(buf));
		GetDlgItemText(IDC_RAW_FIVE, buf, 512);
		entry->setRawFive(Text::fromT(buf));
		GetDlgItemText(IDC_SERVER, buf, 512);
		entry->setIP(Text::fromT(buf));
		GetDlgItemText(IDC_OPCHAT_STR, buf, 512);
		entry->setOpChat(Text::fromT(buf));

                // !SMT!-S
                GetDlgItemText(IDC_CLIENT_ID_BOX, buf, 512);
                entry->setClientId(Text::fromT(buf));
                entry->setOverrideId(IsDlgButtonChecked(IDC_CLIENT_ID) == BST_CHECKED);


		int	ct = -1;
		if(IsDlgButtonChecked(IDC_DEFAULT))
			ct = 0;
		else if(IsDlgButtonChecked(IDC_ACTIVE))
			ct = 1;
		else if(IsDlgButtonChecked(IDC_PASSIVE))
			ct = 2;

		entry->setMode(ct);

		if(Util::strnicmp("adc://", url.c_str(), 6) == 0 || Util::strnicmp("adcs://", url.c_str(), 7) == 0) {
			entry->setHideShare(false);
		}

		FavoriteManager::getInstance()->save();
	}
	EndDialog(wID);
	return 0;
}

LRESULT FavHubProperties::OnTextChanged(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
{
	TCHAR buf[256];

	GetDlgItemText(wID, buf, 256);
	tstring old = buf;

	// Strip '$', '|' and ' ' from text
	TCHAR *b = buf, *f = buf, c;
	while( (c = *b++) != 0 )
	{
		if(c != '$' && c != '|' && (wID == IDC_HUBUSERDESCR || c != ' ') && ( (wID != IDC_HUBNICK && wID != IDC_HUBUSERDESCR && wID != IDC_HUBEMAIL) || (c != '<' && c != '>') ) )
			*f++ = c;
	}

	*f = '\0';

	if(old != buf)
	{
		// Something changed; update window text without changing cursor pos
		CEdit tmp;
		tmp.Attach(hWndCtl);
		int start, end;
		tmp.GetSel(start, end);
		tmp.SetWindowText(buf);
		if(start > 0) start--;
		if(end > 0) end--;
		tmp.SetSel(start, end);
		tmp.Detach();
	}

	return TRUE;
}

// !SMT!-S
LRESULT FavHubProperties::OnChangeId(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        ::EnableWindow(GetDlgItem(IDC_CLIENT_ID_BOX), (IsDlgButtonChecked(IDC_CLIENT_ID) == BST_CHECKED));
        return 0;
}
/**
 * @file
 * $Id: FavHubProperties.cpp,v 1.2 2008/03/10 06:34:23 alexey Exp $
 */
