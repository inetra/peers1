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

#if !defined(UC_HANDLER_H)
#define UC_HANDLER_H

#include "../client/FavoriteManager.h"
#include "../client/StringTokenizer.h"
#include "../client/ClientManager.h"
#include "OMenu.h"

template<class T>
class UCHandler {
public:
	UCHandler() : menuPos(0) { }

	typedef UCHandler<T> thisClass;
	BEGIN_MSG_MAP(thisClass)
		COMMAND_RANGE_HANDLER(IDC_USER_COMMAND, IDC_USER_COMMAND + userCommands.size(), onUserCommand)
	END_MSG_MAP()

	LRESULT onUserCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		dcassert(wID >= IDC_USER_COMMAND);
		size_t n = (size_t)wID - IDC_USER_COMMAND;
		dcassert(n < userCommands.size());

		UserCommand& uc = userCommands[n];

		T* t = static_cast<T*>(this);

		t->runUserCommand(uc);
		return 0;
	}

	void prepareMenu(CMenu& menu, int ctx, const string& hubUrl) {
		prepareMenu(menu, ctx, StringList(1, hubUrl));
	}

	void prepareMenu(CMenu& menu, int ctx, const StringList& hubs) {
		bool op = false;
		userCommands = FavoriteManager::getInstance()->getUserCommands(ctx, hubs, op);
		int n = 0;
		int m = 0;

		menuPos = menu.GetMenuItemCount();
		if(!userCommands.empty()) {
         if (1 /* op && (ctx != UserCommand::CONTEXT_HUB) */) {
				menu.AppendMenu(MF_SEPARATOR);
				menu.AppendMenu(MF_STRING, IDC_GET_USER_RESPONSES, CTSTRING(GET_USER_RESPONSES));
				menu.AppendMenu(MF_STRING, IDC_REPORT, CTSTRING(REPORT));
				menu.AppendMenu(MF_STRING, IDC_CHECKLIST, CTSTRING(CHECK_FILELIST));
				extraItems = 5;
			} else {
				extraItems = 1;
			}
			menu.AppendMenu(MF_SEPARATOR);
			CMenuHandle cur = menu.m_hMenu;
			for(UserCommand::List::iterator ui = userCommands.begin(); ui != userCommands.end(); ++ui) {
				UserCommand& uc = *ui;
				if(uc.getType() == UserCommand::TYPE_SEPARATOR) {
					// Avoid double separators...
					if( (cur.GetMenuItemCount() >= 1) &&
						!(cur.GetMenuState(cur.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR))
					{
						cur.AppendMenu(MF_SEPARATOR);m++;
					}
				} else if(uc.getType() == UserCommand::TYPE_RAW || uc.getType() == UserCommand::TYPE_RAW_ONCE) {
					cur = menu.m_hMenu;
					StringTokenizer<tstring> t(Text::toT(uc.getName()), _T('\\'));
					for(TStringIter i = t.getTokens().begin(); i != t.getTokens().end(); ++i) {
						if(i+1 == t.getTokens().end()) {
							cur.AppendMenu(MF_STRING, IDC_USER_COMMAND+n, i->c_str());m++;
						} else {
							bool found = false;
							TCHAR buf[1024];
							// Let's see if we find an existing item...
							for(int k = 0; k < cur.GetMenuItemCount(); k++) {
								if(cur.GetMenuState(k, MF_BYPOSITION) & MF_POPUP) {
									cur.GetMenuString(k, buf, 1024, MF_BYPOSITION);
									if(Util::stricmp(buf, i->c_str()) == 0) {
										found = true;
										cur = (HMENU)cur.GetSubMenu(k);
									}
								}
							}
							if(!found) {
								HMENU m = CreatePopupMenu();
								cur.AppendMenu(MF_POPUP, (UINT_PTR)m, i->c_str());
								cur = m;
							}
						}
					}
				} else {
					dcasserta(0);
				}
				n++;
			}
		}
	}
	void cleanMenu(OMenu& menu) {
		if(!userCommands.empty()) {
			for(size_t i = 0; i < userCommands.size()+extraItems; ++i) {
				menu.DeleteMenu(menuPos, MF_BYPOSITION);
			}
		}
	}
private:
	UserCommand::List userCommands;
	int menuPos;
	int extraItems;
};

#endif // !defined(UC_HANDLER_H)

/**
 * @file
 * $Id: UCHandler.h,v 1.2 2007/11/28 12:22:43 alexey Exp $
 */
