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

#if !defined(PRIVATE_FRAME_H)
#define PRIVATE_FRAME_H

//[-]PPA [Doxygen 1.5.1] #include "../client/User.h"
#include "../client/ClientManagerListener.h"

#include "UCHandler.h"

#include "ChatCtrl.h"
#include "../peers/CaptionFont.h"
#include "../peers/TextRenderer.h"

#define PM_MESSAGE_MAP 8		// This could be any number, really...

class PrivateFrame : 
  public MDITabChildWindowImpl<PrivateFrame, IDI_PRIVATE_CHAT_HEADER, IDI_PRIVATE_CHAT_OFFLINE>, 
  private ClientManagerListener, 
  public UCHandler<PrivateFrame>, 
  private SettingsManagerListener
{
  friend class PrivateFrameFactory;
private:
  virtual bool isLargeIcon() const { return true; }
public:
	enum {
		USER_UPDATED
	};

	DECLARE_FRAME_WND_CLASS_EX(_T("PrivateFrame"), IDI_PRIVATE_CHAT_HEADER, 0, COLOR_3DFACE);

	typedef MDITabChildWindowImpl<PrivateFrame, IDI_PRIVATE_CHAT_HEADER, IDI_PRIVATE_CHAT_OFFLINE> baseClass;
	typedef UCHandler<PrivateFrame> ucBase;

        BEGIN_MSG_MAP(PrivateFrame)
                MESSAGE_HANDLER(WM_SETFOCUS, onFocus)
                MESSAGE_HANDLER(WM_CREATE, OnCreate)
                MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkGnd)
                MESSAGE_HANDLER(WM_PAINT, OnPaint)
                MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
                MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
                MESSAGE_HANDLER(WM_CLOSE, onClose)
                MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
                MESSAGE_HANDLER(WM_CONTEXTMENU, onContextMenu)
                MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
                COMMAND_ID_HANDLER(IDC_GETLIST, onGetList)
                COMMAND_ID_HANDLER(IDC_MATCH_QUEUE, onMatchQueue)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT, onGrantSlot)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT_HOUR, onGrantSlotHour)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT_DAY, onGrantSlotDay)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT_WEEK, onGrantSlotWeek)
                COMMAND_ID_HANDLER(IDC_GRANTSLOT_PERIOD, onGrantSlotPeriod) // !SMT!-UI
                COMMAND_ID_HANDLER(IDC_UNGRANTSLOT, onUnGrantSlot)
                COMMAND_ID_HANDLER(IDC_ADD_TO_FAVORITES, onAddToFavorites)
                COMMAND_ID_HANDLER(IDC_SEND_MESSAGE, onSendMessage)
                COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)
                COMMAND_ID_HANDLER(ID_EDIT_COPY, onEditCopy)
                COMMAND_ID_HANDLER(ID_EDIT_SELECT_ALL, onEditSelectAll)
                COMMAND_ID_HANDLER(ID_EDIT_CLEAR_ALL, onEditClearAll)
                COMMAND_ID_HANDLER(IDC_COPY_ACTUAL_LINE, onCopyActualLine)
                COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
                COMMAND_ID_HANDLER(IDC_COPY_URL, onCopyURL)
                COMMAND_ID_HANDLER(IDC_EMOT, onEmoticons)
                COMMAND_ID_HANDLER(IDC_WINAMP_SPAM, onWinampSpam)
                // Fix for the tab menu functions - begin
                COMMAND_ID_HANDLER(IDC_REPORT, onOPFunc)
                COMMAND_ID_HANDLER(IDC_CHECKLIST, onOPFunc)
                COMMAND_ID_HANDLER(IDC_GET_USER_RESPONSES, onOPFunc)
                // Fix for the tab menu functions - end
                COMMAND_RANGE_HANDLER(IDC_EMOMENU, IDC_EMOMENU + emoMenu.m_menuItems, onEmoPackChange);
                CHAIN_COMMANDS(ucBase)
                CHAIN_MSG_MAP(baseClass)
                NOTIFY_HANDLER(IDC_CLIENT, EN_LINK, onClientEnLink)
        ALT_MSG_MAP(PM_MESSAGE_MAP)
                MESSAGE_HANDLER(WM_CHAR, onChar)
                MESSAGE_HANDLER(WM_KEYDOWN, onChar)
                MESSAGE_HANDLER(WM_KEYUP, onChar)
		MESSAGE_HANDLER(WM_LBUTTONDBLCLK, onLButton) // !Decker!
        END_MSG_MAP();

        LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
        LRESULT OnEraseBkGnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { return 1; }
        LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT onChar(UINT uMsg, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onMatchQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlotHour(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlotDay(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGrantSlotWeek(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onGrantSlotPeriod(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/); // !SMT!-UI
	LRESULT onUnGrantSlot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onAddToFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT onEditCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditSelectAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onEditClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyActualLine(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClientEnLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onCopyURL(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT onEmoPackChange(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onLButton(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled); // !Decker!

	// Fix for the tab menu functions - begin
	LRESULT onOPFunc(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		switch(wID) {
			case IDC_REPORT: ClientManager::getInstance()->reportUser(replyTo); break;
			case IDC_CHECKLIST:
				try {
					QueueManager::getInstance()->addList(replyTo, QueueItem::FLAG_CHECK_FILE_LIST);
				} catch(const Exception& e) {
					LogManager::getInstance()->message(e.getError());		
				}
			break;
			case IDC_GET_USER_RESPONSES:
				try {
					QueueManager::getInstance()->addTestSUR(replyTo, false);
				} catch(const Exception& e) {
					LogManager::getInstance()->message(e.getError());		
				}
			break;
		}
		return 0;
	}
	// Fix for the tab menu functions - end

  	LRESULT onWinampSpam(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		tstring cmd, param, message, status;
		if(SETTING(MEDIA_PLAYER) == 0) {
		cmd = _T("/winamp");
		} else if(SETTING(MEDIA_PLAYER) == 1) {
			cmd = _T("/wmp");
		} else if(SETTING(MEDIA_PLAYER) == 2) {
			cmd = _T("/itunes");
		} else if(SETTING(MEDIA_PLAYER) == 3) {
			cmd = _T("/mpc");
		} else {
			addClientLine(CTSTRING(NO_MEDIA_SPAM));
			return 0;
		}
		if(WinUtil::checkCommand(cmd, param, message, status)){
			if(!message.empty()) {
				sendMessage(message);
			}
			if(!status.empty()) {
				addClientLine(status);
			}
		}
		return 0;
	}
	
	LRESULT onEmoticons(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);

	void addLine(const tstring& aLine, CHARFORMAT2& cf);
	void addLine(const Identity&, const tstring& aLine);
	void addLine(const Identity&, const tstring& aLine, CHARFORMAT2& cf);
	void onEnter();
	void UpdateLayout(BOOL bResizeBars = TRUE);	
	void runUserCommand(UserCommand& uc);
	void readLog();
	
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
		
	LRESULT onSendMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		onEnter();
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT PrivateFrame::onSpeaker(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /* bHandled */) {
		updateTitle();
		return 0;
	}
	
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
		HWND hWnd = (HWND)lParam;
		HDC hDC = (HDC)wParam;
		if(hWnd == ctrlClient.m_hWnd || hWnd == ctrlMessage.m_hWnd) {
			::SetBkColor(hDC, WinUtil::bgColor);
			::SetTextColor(hDC, WinUtil::textColor);
			return (LRESULT)WinUtil::bgBrush;
		}
		bHandled = FALSE;
		return FALSE;
	}

	LRESULT onFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		ctrlMessage.SetFocus();
		ctrlClient.GoToEnd();
		return 0;
	}
	
	void addClientLine(const tstring& aLine) {
		if(!created) {
			CreateEx(WinUtil::mdiClient);
		}
		ctrlStatus.SetText(0, (_T("[") + Text::toT(Util::getShortTimeString()) + _T("] ") + aLine).c_str());
		if (BOOLSETTING(BOLD_PM)) {
			setDirty();
		}
	}
	
	void sendMessage(const tstring& msg);
	
	UserPtr& getUser() { return replyTo; }
private:
        PrivateFrame(const UserPtr& replyTo_) : replyTo(replyTo_),
                created(false), isoffline(false), curCommandPosition(0),
                m_headerHeight(32),
                m_headerFont(CaptionFont::BOLD),
		ctrlClientContainer(WC_EDIT, this, PM_MESSAGE_MAP), // !Decker!
                ctrlMessageContainer(WC_EDIT, this, PM_MESSAGE_MAP) {
                }
	
	virtual ~PrivateFrame() { }

	bool created;
	ChatCtrl ctrlClient;
	CEdit ctrlMessage;
	CStatusBarCtrl ctrlStatus;

        // OMenu grantMenu; // !SMT!-UI
	OMenu textMenu;
	OMenu tabMenu;
	
    CEmotionMenu emoMenu;
	CButton ctrlEmoticons;
	HBITMAP hEmoticonBmp;

	UserPtr replyTo;
	CContainedWindow ctrlMessageContainer;
	CContainedWindow ctrlClientContainer;

	bool isoffline;

	StringMap ucLineParams;

	void updateTitle();
	
	TStringList prevCommands;
	tstring currentCommand;
	TStringList::size_type curCommandPosition;
        int m_headerHeight;
        CIcon m_headerIcon;
        TextRenderer::TextBlock m_header;
        CaptionFont m_headerFont;

	// ClientManagerListener
        virtual void on(ClientManagerListener::UserUpdated, const UserPtr& aUser) throw() { // !SMT!-fix
                if(aUser == replyTo) // !SMT!-fix
			PostMessage(WM_SPEAKER, USER_UPDATED);
	}
	virtual void on(ClientManagerListener::UserConnected, const UserPtr& aUser) throw() {
		if(aUser == replyTo)
			PostMessage(WM_SPEAKER, USER_UPDATED);
	}
	virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw() {
		if(aUser == replyTo)
			PostMessage(WM_SPEAKER, USER_UPDATED);
	}
	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();
};

#endif // !defined(PRIVATE_FRAME_H)

/**
 * @file
 * $Id: PrivateFrame.h,v 1.11 2008/03/10 07:51:06 alexey Exp $
 */
