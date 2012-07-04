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

#include "PrivateFrame.h"
#include "AGEmotionSetup.h"
#include "TextFrame.h"
#include "ChatBot.h"
#include "UserInfoBase.h"

#include "../client/ClientManager.h"
#include "../client/UploadManager.h"
#include "../client/ShareManager.h"
#include "../client/FavoriteManager.h"
//[-]PPA [Doxygen 1.5.1] #include "../client/QueueManager.h"
#include "EmoticonsDlg.h"
#include "../peers/PrivateFrameFactory.h"
#include "../peers/ControlAdjuster.h"
#include "../peers/Sounds.h"

#define HEADER_ICON_HEIGHT 32
#define HEADER_ICON_WIDTH  32

tstring pSelectedLine = Util::emptyStringT;
tstring pSelectedURL = Util::emptyStringT;

extern CAGEmotionSetup* g_pEmotionsSetup;

LRESULT PrivateFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	ctrlClient.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
                WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_NOHIDESEL | ES_READONLY, WS_EX_CLIENTEDGE, IDC_CLIENT);

	ctrlClientContainer.SubclassWindow(ctrlClient.m_hWnd); // !Decker!

        ctrlClient.LimitText(0);
        ctrlClient.SetFont(WinUtil::font);

        ctrlClient.SetBackgroundColor( SETTING(BACKGROUND_COLOR) );
        ctrlClient.SetAutoURLDetect(false);
        ctrlClient.SetEventMask( ctrlClient.GetEventMask() | ENM_LINK );
        ctrlMessage.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                WS_VSCROLL | (BOOLSETTING(MULTILINE_CHAT_INPUT) ? 0 : ES_AUTOHSCROLL) | ES_MULTILINE | ES_AUTOVSCROLL, WS_EX_CLIENTEDGE); // !Decker!
	
	ctrlMessageContainer.SubclassWindow(ctrlMessage.m_hWnd);

	ctrlMessage.SetFont(WinUtil::font);
	ctrlMessage.SetLimitText(9999);

	ctrlEmoticons.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_FLAT | BS_BITMAP | BS_CENTER, 0, IDC_EMOT);
  	hEmoticonBmp = (HBITMAP) ::LoadImage(_Module.get_m_hInst(), MAKEINTRESOURCE(IDB_EMOTICON), IMAGE_BITMAP, 0, 0, LR_SHARED);

  	ctrlEmoticons.SetBitmap(hEmoticonBmp);

        /* !SMT!-UI a bit refactoring
	grantMenu.CreatePopupMenu();
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT, CTSTRING(GRANT_EXTRA_SLOT));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_HOUR, CTSTRING(GRANT_EXTRA_SLOT_HOUR));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_DAY, CTSTRING(GRANT_EXTRA_SLOT_DAY));
	grantMenu.AppendMenu(MF_STRING, IDC_GRANTSLOT_WEEK, CTSTRING(GRANT_EXTRA_SLOT_WEEK));
	grantMenu.AppendMenu(MF_SEPARATOR);
	grantMenu.AppendMenu(MF_STRING, IDC_UNGRANTSLOT, CTSTRING(REMOVE_EXTRA_SLOT));
        */

	tabMenu.CreatePopupMenu();	
	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
	tabMenu.AppendMenu(MF_SEPARATOR);
	tabMenu.AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
	tabMenu.AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
        tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU)); // !SMT!-UI
        tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::userSummaryMenu, CTSTRING(USER_SUMMARY)); // !SMT!-UI
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));

	PostMessage(WM_SPEAKER, USER_UPDATED);
	created = true;

	ClientManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	readLog();
        m_headerHeight = ControlAdjuster::adjustHeaderHeight(m_hWnd);
        m_headerIcon.LoadIcon(IDI_PRIVATE_CHAT_HEADER);
        m_header.addWords(TSTRING(PRIVATE_CHAT_FRAME_TITLE), m_headerFont, GetSysColor(COLOR_WINDOWTEXT));
        m_header.addWord(Text::toT(replyTo->getFirstNick()), m_headerFont, GREEN_COLOR);

	bHandled = FALSE;
	return 1;
}

LRESULT PrivateFrame::onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if (uMsg != WM_KEYDOWN) {
		switch(wParam) {
			case VK_RETURN:
                                if( WinUtil::isShift() || WinUtil::isCtrl() ||  WinUtil::isAlt() ) {
					bHandled = FALSE;
				}
				break;
		case VK_TAB:
				bHandled = TRUE;
  				break;
  			default:
  				bHandled = FALSE;
				break;
		}
		if ((uMsg == WM_CHAR) && (GetFocus() == ctrlMessage.m_hWnd) && (wParam != VK_RETURN) && (wParam != VK_TAB) && (wParam != VK_BACK)) {
			Sounds::PlaySound(SettingsManager::SOUND_TYPING_NOTIFY);
		}
		return 0;
	}
	switch(wParam) {
	case VK_RETURN:
                if( (GetKeyState(VK_SHIFT) & 0x8000) ||
                        (GetKeyState(VK_CONTROL) & 0x8000) ||
                        (GetKeyState(VK_MENU) & 0x8000) ) {
			bHandled = FALSE;
		} else {
                        if(uMsg == WM_KEYDOWN) {
				onEnter();
			}
                }
		break;
	case VK_UP:
                if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
			//scroll up in chat command history
			//currently beyond the last command?
			if (curCommandPosition > 0) {
				//check whether current command needs to be saved
				if (curCommandPosition == prevCommands.size()) {
					AutoArray<TCHAR> messageContents(ctrlMessage.GetWindowTextLength()+2);
					ctrlMessage.GetWindowText(messageContents, ctrlMessage.GetWindowTextLength()+1);
					currentCommand = tstring(messageContents);
				}
				//replace current chat buffer with current command
				ctrlMessage.SetWindowText(prevCommands[--curCommandPosition].c_str());
			}
		} else {
			bHandled = FALSE;
		}
		break;
	case VK_DOWN:
                if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
			//scroll down in chat command history
			//currently beyond the last command?
			if (curCommandPosition + 1 < prevCommands.size()) {
				//replace current chat buffer with current command
				ctrlMessage.SetWindowText(prevCommands[++curCommandPosition].c_str());
			} else if (curCommandPosition + 1 == prevCommands.size()) {
				//revert to last saved, unfinished command
				ctrlMessage.SetWindowText(currentCommand.c_str());
				++curCommandPosition;
			}
		} else {
			bHandled = FALSE;
		}
		break;
	case VK_HOME:
                if (!prevCommands.empty() && (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
			curCommandPosition = 0;
			AutoArray<TCHAR> messageContents(ctrlMessage.GetWindowTextLength()+2);
			ctrlMessage.GetWindowText(messageContents, ctrlMessage.GetWindowTextLength()+1);
			currentCommand = tstring(messageContents);
			ctrlMessage.SetWindowText(prevCommands[curCommandPosition].c_str());
		} else {
			bHandled = FALSE;
		}
		break;
	case VK_END:
                if ((GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000)) {
			curCommandPosition = prevCommands.size();
			ctrlMessage.SetWindowText(currentCommand.c_str());
		} else {
			bHandled = FALSE;
		}
		break;
	default:
		bHandled = FALSE;
	}
	return 0;
}

// !Decker!
LRESULT PrivateFrame::onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled) {
	
	HWND focus = GetFocus();
	bHandled = false;
	if(focus == ctrlClient.m_hWnd) {
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		tstring x;

		int i = ctrlClient.CharFromPos(pt);
		int line = ctrlClient.LineFromChar(i);
		int c = LOWORD(i) - ctrlClient.LineIndex(line);
		int len = ctrlClient.LineLength(i) + 1;
		if(len < 3) {
			return 0;
		}

		TCHAR* buf = new TCHAR[len];
		ctrlClient.GetLine(line, buf, len);
		x = tstring(buf, len-1);
		delete[] buf;

		string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
		
		if(start == string::npos)
			start = 0;
		else
			start++;
		
		string::size_type end = x.find_first_of(_T(" >\t"), start+1);
		
			if(end == string::npos) // get EOL as well
				end = x.length();
			else if(end == start + 1)
				return 0;

			// Nickname click, let's see if we can find one like it in the name list...
			tstring nick = x.substr(start, end - start);
			
			// ник в чат

			CAtlString sUser = nick.c_str();
			CAtlString sText = "";
			int iSelBegin, iSelEnd;
			ctrlMessage.GetSel(iSelBegin, iSelEnd);
			ctrlMessage.GetWindowText(sText);

			 if((iSelBegin == 0) && (iSelEnd == 0)) {
				  if(sText.GetLength() == 0) {
					   sUser += ": ";
					   ctrlMessage.SetWindowText(sUser);
					   ctrlMessage.SetFocus();
					   ctrlMessage.SetSel(ctrlMessage.GetWindowTextLength(), ctrlMessage.GetWindowTextLength());
				  } else {
					   sUser += ": ";
					   ctrlMessage.ReplaceSel(sUser);
					   ctrlMessage.SetFocus();
				  }
			 } else {
				  sUser += " ";
				  ctrlMessage.ReplaceSel(sUser);
				  ctrlMessage.SetFocus();
			 }


	}

return 0;
}

void PrivateFrame::onEnter()
{
	bool resetText = true;

	if(ctrlMessage.GetWindowTextLength() > 0) {
		AutoArray<TCHAR> msg(ctrlMessage.GetWindowTextLength()+1);
		ctrlMessage.GetWindowText(msg, ctrlMessage.GetWindowTextLength()+1);
		tstring s(msg, ctrlMessage.GetWindowTextLength());

		// save command in history, reset current buffer pointer to the newest command
		curCommandPosition = prevCommands.size();		//this places it one position beyond a legal subscript
		if (!curCommandPosition || curCommandPosition > 0 && prevCommands[curCommandPosition - 1] != s) {
			++curCommandPosition;
			prevCommands.push_back(s);
		}
		currentCommand = Util::emptyStringT;

		// Process special commands
		if(s[0] == '/') {
			tstring m = s;
                        s = s.substr(1);
			tstring param;
			tstring message;
			tstring status;
			if(WinUtil::checkCommand(s, param, message, status)) {
				if(!message.empty()) {
					sendMessage(message);
				}
				if(!status.empty()) {
					addClientLine(status);
				}
			} else if((Util::stricmp(s.c_str(), _T("clear")) == 0) || (Util::stricmp(s.c_str(), _T("cls")) == 0) || (Util::stricmp(s.c_str(), _T("c")) == 0)) {
				ctrlClient.SetWindowText(_T(""));
			} else if(Util::stricmp(s.c_str(), _T("grant")) == 0) {
				UploadManager::getInstance()->reserveSlot(getUser(), 600);
				addClientLine(TSTRING(SLOT_GRANTED));
			} else if(Util::stricmp(s.c_str(), _T("close")) == 0) {
				PostMessage(WM_CLOSE);
			} else if((Util::stricmp(s.c_str(), _T("favorite")) == 0) || (Util::stricmp(s.c_str(), _T("fav")) == 0)) {
				FavoriteManager::getInstance()->addFavoriteUser(getUser());
				addClientLine(TSTRING(FAVORITE_USER_ADDED));
			} else if((Util::stricmp(s.c_str(), _T("getlist")) == 0)||(Util::stricmp(s.c_str(), _T("gl")) == 0)) {
				BOOL bTmp;
				onGetList(0,0,0,bTmp);
			} else if(Util::stricmp(s.c_str(), _T("log")) == 0) {
				StringMap params;

				params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(replyTo->getCID()));
				params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(replyTo->getCID()));
				params["userCID"] = replyTo->getCID().toBase32(); 
				params["userNI"] = replyTo->getFirstNick();
				params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
				WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false))));
			} else if(Util::stricmp(s.c_str(), _T("stats")) == 0) {
				addLine(Text::toT(WinUtil::generateStats()), WinUtil::m_ChatTextGeneral);
			} else if(Util::stricmp(s.c_str(), _T("pubstats")) == 0) {
				sendMessage(Text::toT(WinUtil::generateStats()));
			} else if(Util::stricmp(s.c_str(), _T("help")) == 0 || Util::stricmp(s.c_str(), _T("h")) == 0) {
				addLine(_T("*** ") + WinUtil::getChatHelp() + _T(""), WinUtil::m_ChatTextSystem);
			} else if(Util::stricmp(s.c_str(), _T("me")) == 0) {
				sendMessage(m);
			} else {
				if(replyTo->isOnline()) {
					sendMessage(tstring(m));
				} else {
					ctrlStatus.SetText(0, CTSTRING(USER_WENT_OFFLINE));
					resetText = false;
				}
			}
		} else {
			if(replyTo->isOnline()) {
//[-]PPA				if(BOOLSETTING(CZCHARS_DISABLE))
//[-]PPA					s = Text::toT(WinUtil::disableCzChars(Text::fromT(s)));

				sendMessage(s);
			} else {
				ctrlStatus.SetText(0, CTSTRING(USER_WENT_OFFLINE));
				resetText = false;
			}
		}
		if(resetText)
			ctrlMessage.SetWindowText(_T(""));
	} 
}

void PrivateFrame::sendMessage(const tstring& msg) {
	ClientManager::getInstance()->privateMessage(replyTo, Text::fromT(msg));
}

LRESULT PrivateFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	DeleteObject(hEmoticonBmp);
	ClientManager::getInstance()->removeListener(this);
	SettingsManager::getInstance()->removeListener(this);
               WinUtil::UnlinkStaticMenus(tabMenu); // !SMT!-UI
	bHandled = FALSE;
	return 0;
}

void PrivateFrame::addLine(const tstring& aLine, CHARFORMAT2& cf) {
	Identity i = Identity(NULL, 0);
    addLine(i, aLine, cf);
}

void PrivateFrame::addLine(const Identity& from, const tstring& aLine) {
	addLine(from, aLine, WinUtil::m_ChatTextGeneral );
}

void PrivateFrame::addLine(const Identity& from, const tstring& aLine, CHARFORMAT2& cf) {
	if(!created) {
		if(BOOLSETTING(POPUNDER_PM))
			WinUtil::hiddenCreateEx(this);
		else
			CreateEx(WinUtil::mdiClient);
	}
	ctrlClient.AdjustTextSize();


	string extra;
        if(BOOLSETTING(IP_IN_CHAT) || BOOLSETTING(COUNTRY_IN_CHAT)) {
                if(ClientManager::getInstance()->isOp(ClientManager::getInstance()->getMe(), Util::toString(ClientManager::getInstance()->getHubs(replyTo->getCID())))) // is there a better way to do this?
                        extra = WinUtil::getIpCountryForChat(from.getIp(), BOOLSETTING(TIME_STAMPS));
        }

	CRect r;
	ctrlClient.GetClientRect(r);

	tstring line = aLine;
	string::size_type i = line.find('>', 2);
	if(i != string::npos) {
		tstring nick = line.substr(1, i-1);
		if((line.size() > i+5) && (Util::stricmp(line.substr(i+2, 3), _T("/me")) == 0 || Util::stricmp(line.substr(i+2, 3), _T("+me")) == 0)) {
			line = _T("* ") + nick + line.substr(i+5);
		}
	}

	if(BOOLSETTING(LOG_PRIVATE_CHAT)) {
		StringMap params;
		params["message"] = Text::fromT(line);
		params["extra"] = extra;
		params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(replyTo->getCID()));
		params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(replyTo->getCID()));
		params["userCID"] = replyTo->getCID().toBase32(); 
		params["userNI"] = replyTo->getFirstNick();
		params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
		LOG(LogManager::PM, params);
	}

	if(BOOLSETTING(TIME_STAMPS)) {
		ctrlClient.AppendText(from, Text::toT(SETTING(NICK)), Text::toT("[" + Util::getShortTimeString() + extra + "] "), line.c_str(), cf);
	} else {
		ctrlClient.AppendText(from, Text::toT(SETTING(NICK)), !extra.empty() ? Text::toT("[" + extra + "] ") : _T(""), line.c_str(), cf);
	}
	addClientLine(CTSTRING(LAST_CHANGE) +  Text::toT(Util::getTimeString()));

	if (BOOLSETTING(BOLD_PM)) {
		setDirty();
	}
}

LRESULT PrivateFrame::onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.Copy();
	return 0;
}

LRESULT PrivateFrame::onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.SetSelAll();
	return 0;
}

LRESULT PrivateFrame::onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ctrlClient.SetWindowText(_T(""));
	return 0;
}

LRESULT PrivateFrame::onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (pSelectedLine != _T("")) {
		WinUtil::setClipboard(pSelectedLine);
	}
	return 0;
}

LRESULT PrivateFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	prepareMenu(tabMenu, UserCommand::CONTEXT_CHAT, ClientManager::getInstance()->getHubs(replyTo->getCID()));
	if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
	tabMenu.InsertSeparatorFirst(Text::toT(replyTo->getFirstNick()));	

        WinUtil::clearSummaryMenu(); // !SMT!-UI
        UserInfoBase(replyTo).addSummary(); // !SMT!-UI

	tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	tabMenu.RemoveFirstItem();
	tabMenu.DeleteMenu(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION);
	tabMenu.DeleteMenu(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION);
	cleanMenu(tabMenu);
	return TRUE;
}

void PrivateFrame::runUserCommand(UserCommand& uc) {

	if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
		return;

	StringMap ucParams = ucLineParams;

	ClientManager::getInstance()->userCommand(replyTo, uc, ucParams, true);
}

LRESULT PrivateFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(replyTo, QueueItem::FLAG_CLIENT_VIEW);
	} catch(const Exception& e) {
		addClientLine(Text::toT(e.getError()));
	}
	return 0;
}

LRESULT PrivateFrame::onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	try {
		QueueManager::getInstance()->addList(replyTo, QueueItem::FLAG_MATCH_QUEUE);
	} catch(const Exception& e) {
		addClientLine(Text::toT(e.getError()));
	}
	return 0;
}

LRESULT PrivateFrame::onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->reserveSlot(replyTo, 600);
	return 0;
}

LRESULT PrivateFrame::onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->reserveSlot(replyTo, 3600);
	return 0;
}

LRESULT PrivateFrame::onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->reserveSlot(replyTo, 24*3600);
	return 0;
}

LRESULT PrivateFrame::onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->reserveSlot(replyTo, 7*24*3600);
	return 0;
}

// !SMT!-UI
LRESULT PrivateFrame::onGrantSlotPeriod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        uint32_t time = UserInfoBase::inputSlotTime();
        if (time)
                UploadManager::getInstance()->reserveSlot(replyTo, time);
        return 0;
}

LRESULT PrivateFrame::onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UploadManager::getInstance()->unreserveSlot(replyTo);
	return 0;
}

LRESULT PrivateFrame::onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	FavoriteManager::getInstance()->addFavoriteUser(replyTo);
	return 0;
}

void PrivateFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
  CRect rect;
  GetClientRect(&rect);
  // position bars and offset their dimensions
  UpdateBarsPosition(rect, bResizeBars);
  m_header.doLayout(m_hWnd, rect.Width());
  rect.top += m_headerHeight;

  if(ctrlStatus.IsWindow()) {
    CRect sr;
    int w[1];
    ctrlStatus.GetClientRect(sr);

    w[0] = sr.right - 16;

    ctrlStatus.SetParts(1, w);
  }

  int h = WinUtil::fontHeight + 4;
  int chat_columns = BOOLSETTING(MULTILINE_CHAT_INPUT)? 2 : 1; // !Decker!

  CRect rc = rect;
  rc.bottom -= h*chat_columns + 10; // !Decker!
  ctrlClient.MoveWindow(rc);

  rc = rect;
  rc.bottom -= 2;
  rc.top = rc.bottom - h*chat_columns - 5; // !Decker!
  rc.left +=2;
  rc.right -= 2 + 24;
  ctrlMessage.MoveWindow(rc);

  rc.top += h*(chat_columns-1); // !Decker!
  rc.left = rc.right + 2;
  rc.right += 24;

  ctrlEmoticons.MoveWindow(rc);
}

void PrivateFrame::updateTitle() {
	pair<tstring, bool> hubs = WinUtil::getHubNames(replyTo);
        bool banIcon = FavoriteManager::getInstance()->hasBan(replyTo) || FavoriteManager::getInstance()->hasIgnore(replyTo); // !SMT!-UI
	if(hubs.second) {	
                if (banIcon) // !SMT!-UI
                        setCustomIcon(WinUtil::banIconOnline);
                else
		unsetIconState();
		if(isoffline) {
			tstring status = _T(" *** ") + TSTRING(USER_WENT_ONLINE) + _T(" [") + Text::toT(replyTo->getFirstNick() + " - ") + hubs.first + _T("] ***");
			if(BOOLSETTING(STATUS_IN_CHAT)) {
				addLine(status, WinUtil::m_ChatTextServer);
			} else {
				addClientLine(status);
			}
		}
		if(replyTo->getLastHubName().empty())
			replyTo->setLastHubName(Text::fromT(hubs.first)); // anyone find more elegant way to do this, feel free to tell me...
		isoffline = false;
	} else {
                if (banIcon) // !SMT!-UI
                        setCustomIcon(WinUtil::banIconOffline);
                else
		setIconState();
		tstring status = _T(" *** ") + TSTRING(USER_WENT_OFFLINE) + _T(" [") + Text::toT(replyTo->getFirstNick() + " - " + replyTo->getLastHubName() + "] ***");
		if(BOOLSETTING(STATUS_IN_CHAT)) {
			addLine(status, WinUtil::m_ChatTextServer);
		} else {
			addClientLine(status);
		}
		isoffline = true;
	}
	SetWindowText((Text::toT(replyTo->getFirstNick()) + _T(" - ") + hubs.first).c_str());
}

LRESULT PrivateFrame::onContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	bHandled = FALSE;

	POINT p;
	p.x = GET_X_LPARAM(lParam);
	p.y = GET_Y_LPARAM(lParam);
	::ScreenToClient(ctrlClient.m_hWnd, &p);

	POINT cpt;
	GetCursorPos(&cpt);

	CRect rc;            // client area of window 
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click
	OMenu Mnu;

	pSelectedLine = _T("");

	bool bHitURL = ctrlClient.HitURL();
	if (!bHitURL)
		pSelectedURL = _T("");

        if(reinterpret_cast<HWND>(wParam) == ctrlEmoticons) {
			    emoMenu.CreateEmotionMenu(pt,m_hWnd,IDC_EMOMENU); //[+]PPA 
                return TRUE;
        }

	int i = ctrlClient.CharFromPos(p);
	int line = ctrlClient.LineFromChar(i);
	int c = LOWORD(i) - ctrlClient.LineIndex(line);
	int len = ctrlClient.LineLength(i) + 1;
	if ( len < 3 )
		return 0;
	TCHAR* buf = new TCHAR[len];
	ctrlClient.GetLine(line, buf, len);
	tstring x = tstring(buf, len-1);
	delete[] buf;
	string::size_type start = x.find_last_of(_T(" <\t\r\n"), c);
	if (start == string::npos) { start = 0; }
	tstring nick = Text::toT(replyTo->getFirstNick());
	if (x.substr(start, (nick.length() + 2) ) == (_T("<") + nick + _T(">"))) {
		if(!replyTo->isOnline()) {
			return S_OK;
		}

                WinUtil::clearSummaryMenu(); // !SMT!-UI
                UserInfoBase(replyTo).addSummary(); // !SMT!-UI

		prepareMenu(tabMenu, UserCommand::CONTEXT_CHAT, ClientManager::getInstance()->getHubs(replyTo->getCID()));
		if(!(tabMenu.GetMenuState(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {	
			tabMenu.AppendMenu(MF_SEPARATOR);
		}
		tabMenu.InsertSeparatorFirst(nick);
		tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));
		tabMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cpt.x, cpt.y, m_hWnd);
		tabMenu.RemoveFirstItem();
		tabMenu.DeleteMenu(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION);
		tabMenu.DeleteMenu(tabMenu.GetMenuItemCount()-1, MF_BYPOSITION);
		cleanMenu(tabMenu);
		bHandled = TRUE;
	} else {
		if (textMenu.m_hMenu != NULL) {
			textMenu.DestroyMenu();
			textMenu.m_hMenu = NULL;
		}

		textMenu.CreatePopupMenu();
		textMenu.AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
		textMenu.AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CTSTRING(COPY_LINE));
		if(pSelectedURL != _T(""))
			textMenu.AppendMenu(MF_STRING, IDC_COPY_URL, CTSTRING(COPY_URL));
		textMenu.AppendMenu(MF_SEPARATOR);
		textMenu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
		textMenu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));

		pSelectedLine = ctrlClient.LineFromPos(p);
		textMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, cpt.x, cpt.y, m_hWnd);
		bHandled = TRUE;
	}
	return S_OK;
}

LRESULT PrivateFrame::onClientEnLink(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	ENLINK* pEL = (ENLINK*)pnmh;

	if ( pEL->msg == WM_LBUTTONUP ) {
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR* sURLTemp = new TCHAR[(lEnd - lBegin)+1];
		if(sURLTemp) {
			ctrlClient.GetTextRange(lBegin, lEnd, sURLTemp);
			tstring sURL = sURLTemp;
			WinUtil::openLink(sURL);
			delete[] sURLTemp;
		}
	} else if ( pEL->msg == WM_RBUTTONUP ) {
		pSelectedURL = _T("");
		long lBegin = pEL->chrg.cpMin, lEnd = pEL->chrg.cpMax;
		TCHAR* sURLTemp = new TCHAR[(lEnd - lBegin)+1];
		if(sURLTemp) {
			ctrlClient.GetTextRange(lBegin, lEnd, sURLTemp);
			pSelectedURL = sURLTemp;
			delete[] sURLTemp;
		}

		ctrlClient.SetSel( lBegin, lEnd );
		ctrlClient.InvalidateRect( NULL );
		return 0;
	}
	return 0;
}

void PrivateFrame::readLog() {
	if (SETTING(SHOW_LAST_LINES_LOG) == 0) return;

	StringMap params;
	
	params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(replyTo->getCID()));
	params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(replyTo->getCID()));
	params["userCID"] = replyTo->getCID().toBase32(); 
	params["userNI"] = replyTo->getFirstNick();
	params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();
		
	string path = Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false));

	try {
		File f(path, File::READ, File::OPEN);
		
		int64_t size = f.getSize();

		if(size > 32*1024) {
			f.setPos(size - 32*1024);
		}
		string buf = f.read(32*1024);
		StringList lines;

		if(Util::strnicmp(buf.c_str(), "\xef\xbb\xbf", 3) == 0)
			lines = StringTokenizer<string>(buf.substr(3), "\r\n").getTokens();
		else
			lines = StringTokenizer<string>(buf, "\r\n").getTokens();

		int linesCount = lines.size();

		int i = linesCount > (SETTING(SHOW_LAST_LINES_LOG) + 1) ? linesCount - SETTING(SHOW_LAST_LINES_LOG) : 0;

		for(; i < linesCount; ++i){
			ctrlClient.AppendText(Identity(NULL, 0), _T("- "), _T(""), (Text::toT(lines[i])).c_str(), WinUtil::m_ChatTextLog, true);
		}

		f.close();
	} catch(const FileException&){
	}
}

LRESULT PrivateFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {	
	StringMap params;
	params["hubNI"] = Util::toString(ClientManager::getInstance()->getHubNames(replyTo->getCID()));
	params["hubURL"] = Util::toString(ClientManager::getInstance()->getHubs(replyTo->getCID()));
	params["userCID"] = replyTo->getCID().toBase32(); 
	params["userNI"] = replyTo->getFirstNick();
	params["myCID"] = ClientManager::getInstance()->getMe()->getCID().toBase32();

	string file = Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false));
	if(Util::fileExists(file)) {
		if(BOOLSETTING(OPEN_LOGS_INTERNAL) == false) {
		ShellExecute(NULL, NULL, Text::toT(file).c_str(), NULL, NULL, SW_SHOWNORMAL);
	} else {
			TextFrame::openWindow(Text::toT(file).c_str());
		}
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_USER), CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}	

	return 0;
}

LRESULT PrivateFrame::onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (pSelectedURL != _T("")) {
		WinUtil::setClipboard(pSelectedURL);
	}
	return 0;
}

void PrivateFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
	ctrlClient.SetBackgroundColor(WinUtil::bgColor);
	RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

LRESULT PrivateFrame::onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
        TCHAR buf[256];
        emoMenu.GetMenuString(wID, buf, 256, MF_BYCOMMAND);
        if (buf!=Text::toT(SETTING(EMOTICONS_FILE))) {
                SettingsManager::getInstance()->set(SettingsManager::EMOTICONS_FILE, Text::fromT(buf));
                delete g_pEmotionsSetup;
                g_pEmotionsSetup = NULL;
                g_pEmotionsSetup = new CAGEmotionSetup;
                if ((g_pEmotionsSetup == NULL)||
                        (!g_pEmotionsSetup->Create())){
                        dcassert(FALSE);
                        return -1;
                }
        }
        return 0;
}

LRESULT PrivateFrame::onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled) {
  if (hWndCtl != ctrlEmoticons.m_hWnd) {
    bHandled = false;
    return 0;
  }
  EmoticonsDlg dlg;
  ctrlEmoticons.GetWindowRect(dlg.pos);
  dlg.DoModal(m_hWnd);
  if (!dlg.result.empty()) {
    // !BUGMASTER!
    int start, end;
    int len = ctrlMessage.GetWindowTextLength();
    ctrlMessage.GetSel(start, end);
    TCHAR* message = new TCHAR[len + 1];
    ctrlMessage.GetWindowText(message, len + 1);
    tstring s1(message, start);
    tstring s2(message + end, len - end);
    delete[] message;
    ctrlMessage.SetWindowText((s1 + dlg.result + s2).c_str());
    ctrlMessage.SetFocus();
    start += dlg.result.length();
    ctrlMessage.SetSel(start, start);
    // end !BUGMASTER!
  }
  return 0;
}

LRESULT PrivateFrame::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
  CRect r;
  GetClientRect(r);
  UpdateBarsPosition(r, FALSE);
  CPaintDC dc(m_hWnd);
  const HBRUSH brush = GetSysColorBrush(COLOR_BTNFACE);
  dc.FillRect(r, brush);
  const int delta = (m_headerHeight - HEADER_ICON_HEIGHT) / 2;
  m_headerIcon.DrawIconEx(dc, r.left + delta, r.top + delta, HEADER_ICON_WIDTH, HEADER_ICON_HEIGHT, 0, brush, DI_NORMAL);
  dc.SetBkColor(GetSysColor(COLOR_BTNFACE));
  m_header.draw((HDC) dc, r.left + delta + HEADER_ICON_WIDTH + 2, r.top + (m_headerHeight - m_header.getHeight()) / 2);
  return 0;
}

/**
 * @file
 * $Id: PrivateFrame.cpp,v 1.12 2008/03/10 06:55:27 alexey Exp $
 */
