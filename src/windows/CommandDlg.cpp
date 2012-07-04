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
#include "../client/UserCommand.h"
#include "../client/NmdcHub.h"
#include "CommandDlg.h"

WinUtil::TextItem CommandDlg::texts[] = {
	{ IDOK, ResourceManager::OK },
	{ IDCANCEL, ResourceManager::CANCEL },
	{ IDC_SETTINGS_TYPE, ResourceManager::USER_CMD_TYPE },
	{ IDC_SETTINGS_SEPARATOR, ResourceManager::SEPARATOR },
	{ IDC_SETTINGS_RAW, ResourceManager::USER_CMD_RAW },
	{ IDC_SETTINGS_CHAT, ResourceManager::USER_CMD_CHAT },
	{ IDC_SETTINGS_PM, ResourceManager::USER_CMD_PM },
	{ IDC_SETTINGS_CONTEXT, ResourceManager::USER_CMD_CONTEXT },
	{ IDC_SETTINGS_HUB_MENU, ResourceManager::USER_CMD_HUB_MENU },
	{ IDC_SETTINGS_USER_MENU, ResourceManager::USER_CMD_USER_MENU },
	{ IDC_SETTINGS_SEARCH_MENU, ResourceManager::USER_CMD_SEARCH_MENU },
	{ IDC_SETTINGS_FILELIST_MENU, ResourceManager::USER_CMD_FILELIST_MENU },
	{ IDC_SETTINGS_PARAMETERS, ResourceManager::USER_CMD_PARAMETERS },
	{ IDC_SETTINGS_NAME, ResourceManager::HUB_NAME },
	{ IDC_SETTINGS_COMMAND, ResourceManager::USER_CMD_COMMAND },
	{ IDC_SETTINGS_HUB, ResourceManager::USER_CMD_HUB },
	{ IDC_SETTINGS_TO, ResourceManager::USER_CMD_TO },
	{ IDC_SETTINGS_ONCE, ResourceManager::USER_CMD_ONCE },
	{ IDC_USER_CMD_EXAMPLE, ResourceManager::USER_CMD_PREVIEW },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT CommandDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// Translate
	SetWindowText(CTSTRING(USER_CMD_WINDOW));

#define ATTACH(id, var) var.Attach(GetDlgItem(id))
	ATTACH(IDC_RESULT, ctrlResult);
	ATTACH(IDC_NAME, ctrlName);
	ATTACH(IDC_HUB, ctrlHub);
	ATTACH(IDC_SETTINGS_SEPARATOR, ctrlSeparator);
	ATTACH(IDC_SETTINGS_RAW, ctrlRaw);
	ATTACH(IDC_SETTINGS_CHAT, ctrlChat);
	ATTACH(IDC_SETTINGS_PM, ctrlPM);
	ATTACH(IDC_SETTINGS_ONCE, ctrlOnce);
	ATTACH(IDC_SETTINGS_HUB_MENU, ctrlHubMenu);
	ATTACH(IDC_SETTINGS_USER_MENU, ctrlUserMenu);
	ATTACH(IDC_SETTINGS_SEARCH_MENU, ctrlSearchMenu);
	ATTACH(IDC_SETTINGS_FILELIST_MENU, ctrlFilelistMenu);
	ATTACH(IDC_NICK, ctrlNick);
	ATTACH(IDC_COMMAND, ctrlCommand);

	WinUtil::translate(*this, texts);
	SetDlgItemText(IDC_COMMAND_DESCRIPTION, _T("\
Command Types:\r\n\
Separator: Adds a separator to the menu\r\n\
Raw: Sends raw command to the hub (experts only, end it with '|'!)\r\n\
Chat: Sends command as if you were typing it in the chat\r\n\
PM: Sends command as if you sent it by pm\r\n\
Contexts determine where the command is shown:\r\n\
Hub Menu: Hub tab (at the bottom of the screen) right-click menu\r\n\
Chat Menu: User right-click menu in chat and PM tab menu\r\n\
Search Menu: Search right-click menu\r\n\
Parameters:\r\n\
Name: Name (use '\\' to create submenus)\r\n\
Command: Command text (may contain parameters)\r\n")
_T("Hub: Hub ip as typed when connecting (empty = all hubs, \"op\" = hubs where you're op)\r\n\
To: PM recipient\r\n\
Only once: Send only once per user from search frame\r\n\
In the parameters, you can use %[xxx] variables and date/time specifiers (%Y, %m, ...). The following are available:\r\n\
%[myNI]: your own nick\r\n\
%[userNI]: the users nick (user && search context only)\r\n\
%[userTAG]: user tag (user && search context only)\r\n\
%[userDE]: user description (user && search context only)\r\n\
%[userEM]: user email (user && search context only)\r\n\
%[userSS]: user shared bytes (exact) (user && search context only)\r\n\
%[userSSshort]: user shared bytes (formatted) (user && search context only)\r\n\
%[userI4]: user ip (if supported by hub)\r\n\
%[fileFN]: filename (search context only)\r\n\
%[line:reason]: opens up a window asking for \"reason\"\
"));

	if(type == UserCommand::TYPE_SEPARATOR) {
		ctrlSeparator.SetCheck(BST_CHECKED);
	} else {
		// More difficult, determine type by what it seems to be...
		if((_tcsncmp(command.c_str(), _T("$To: "), 5) == 0) &&
			(command.find(_T(" From: %[myNI] $<%[myNI]> ")) != string::npos ||
			command.find(_T(" From: %[mynick] $<%[mynick]> ")) != string::npos) &&
			command.find(_T('|')) == command.length() - 1) // if it has | anywhere but the end, it is raw
		{
			string::size_type i = command.find(_T(' '), 5);
			dcassert(i != string::npos);
			tstring to = command.substr(5, i-5);
			string::size_type cmd_pos = command.find(_T('>'), 5) + 2;
			tstring cmd = Text::toT(NmdcHub::validateMessage(Text::fromT(command.substr(cmd_pos, command.length()-cmd_pos-1)), true));
			ctrlPM.SetCheck(BST_CHECKED);
			ctrlNick.SetWindowText(to.c_str());
			ctrlCommand.SetWindowText(cmd.c_str());
		} else if(((_tcsncmp(command.c_str(), _T("<%[mynick]> "), 12) == 0) ||
			(_tcsncmp(command.c_str(), _T("<%[myNI]> "), 10) == 0)) &&
			command[command.length()-1] == '|') 
		{
			// Looks like a chat thing...
			string::size_type cmd_pos = command.find(_T('>')) + 2;
			tstring cmd = Text::toT(NmdcHub::validateMessage(Text::fromT(command.substr(cmd_pos, command.length()-cmd_pos-1)), true));
			ctrlChat.SetCheck(BST_CHECKED);
			ctrlCommand.SetWindowText(cmd.c_str());
		} else {
			tstring cmd = command;
			ctrlRaw.SetCheck(BST_CHECKED);
			ctrlCommand.SetWindowText(cmd.c_str());
		}
		if(type == UserCommand::TYPE_RAW_ONCE) {
			ctrlOnce.SetCheck(BST_CHECKED);
			type = 1;
		}
	}

	ctrlHub.SetWindowText(hub.c_str());
	ctrlName.SetWindowText(name.c_str());

	if(ctx & UserCommand::CONTEXT_HUB)
		ctrlHubMenu.SetCheck(BST_CHECKED);
	if(ctx & UserCommand::CONTEXT_CHAT)
		ctrlUserMenu.SetCheck(BST_CHECKED);
	if(ctx & UserCommand::CONTEXT_SEARCH)
		ctrlSearchMenu.SetCheck(BST_CHECKED);
	if(ctx & UserCommand::CONTEXT_FILELIST)
		ctrlFilelistMenu.SetCheck(BST_CHECKED);
	
	updateControls();
	updateCommand();
	ctrlResult.SetWindowText(command.c_str());

	ctrlSeparator.SetFocus();
	
	CenterWindow(GetParent());
	return FALSE;
}

LRESULT CommandDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	updateContext();
	if(wID == IDOK) {
		TCHAR buf[256];

		if((type != 0) && 
			((ctrlName.GetWindowTextLength() == 0) || (ctrlCommand.GetWindowTextLength()== 0)))
		{
			MessageBox(_T("Name and command must not be empty"));
			return 0;
		}

#define GET_TEXT(id, var) \
	GetDlgItemText(id, buf, 256); \
	var = buf;

		GET_TEXT(IDC_NAME, name);
		GET_TEXT(IDC_HUB, hub);

		if(type != 0) {
			type = (ctrlOnce.GetCheck() == BST_CHECKED) ? 2 : 1;
		}
	}
	EndDialog(wID);
	return 0;
}

LRESULT CommandDlg::onChange(WORD , WORD , HWND , BOOL& ) {
	updateCommand();
	ctrlResult.SetWindowText(command.c_str());
	return 0;
}

LRESULT CommandDlg::onType(WORD , WORD, HWND , BOOL& ) {
	updateType();
	updateCommand();
	ctrlResult.SetWindowText(command.c_str());
	updateControls();
	return 0;
}

void CommandDlg::updateContext() {
	ctx = 0;
	if(ctrlHubMenu.GetCheck() & BST_CHECKED)
		ctx |= UserCommand::CONTEXT_HUB;
	if(ctrlUserMenu.GetCheck() & BST_CHECKED)
		ctx |= UserCommand::CONTEXT_CHAT;
	if(ctrlSearchMenu.GetCheck() & BST_CHECKED)
		ctx |= UserCommand::CONTEXT_SEARCH;
	if(ctrlFilelistMenu.GetCheck() & BST_CHECKED)
		ctx |= UserCommand::CONTEXT_FILELIST;
}


void CommandDlg::updateType() {
	if(ctrlSeparator.GetCheck() == BST_CHECKED) {
		type = 0;
	} else if(ctrlRaw.GetCheck() == BST_CHECKED) {
		type = 1;
	} else if(ctrlChat.GetCheck() == BST_CHECKED) {
		type = 2;
	} else if(ctrlPM.GetCheck() == BST_CHECKED) {
		type = 3;
	}
}

void CommandDlg::updateCommand() {
	static const size_t BUF_LEN = 1024;
	TCHAR buf[BUF_LEN];
	if(type == 0) {
		command.clear();
	} else if(type == 1) {
		ctrlCommand.GetWindowText(buf, BUF_LEN-1);
		command = buf;
	} else if(type == 2) {
		ctrlCommand.GetWindowText(buf, BUF_LEN - 1);
		command = Text::toT("<%[myNI]> " + NmdcHub::validateMessage(Text::fromT(buf), false) + "|");
	} else if(type == 3) {
		ctrlNick.GetWindowText(buf, BUF_LEN - 1);
		tstring to(buf);
		ctrlCommand.GetWindowText(buf, BUF_LEN - 1);
		command = _T("$To: ") + to + _T(" From: %[myNI] $<%[myNI]> ") + Text::toT(NmdcHub::validateMessage(Text::fromT(buf), false)) + _T("|");
	}
}

void CommandDlg::updateControls() {
	switch(type) {
		case 0:
			ctrlName.EnableWindow(FALSE);
			ctrlCommand.EnableWindow(FALSE);
			ctrlNick.EnableWindow(FALSE);
			break;
		case 1:
		case 2:
			ctrlName.EnableWindow(TRUE);
			ctrlCommand.EnableWindow(TRUE);
			ctrlNick.EnableWindow(FALSE);
			break;
		case 3:
			ctrlName.EnableWindow(TRUE);
			ctrlCommand.EnableWindow(TRUE);
			ctrlNick.EnableWindow(TRUE);
			break;
	}
}

/**
* @file
* $Id: CommandDlg.cpp,v 1.3 2008/03/10 07:51:06 alexey Exp $
*/
