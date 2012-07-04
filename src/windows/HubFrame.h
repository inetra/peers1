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

#if !defined(HUB_FRAME_H)
#define HUB_FRAME_H

#include "TypedListViewCtrl.h"
#include "ChatCtrl.h"
#include "EmoticonsDlg.h"

#include "../client/User.h"
#include "../client/ClientManager.h"
#include "../client/TimerManager.h"
#include "../client/FastAlloc.h"
#include "../client/DirectoryListing.h"
#include "../client/TaskQueue.h"

#include "UCHandler.h"
#include "UserInfo.h"
#include "../peers/UserListControl.h"
#include "../peers/ChatControl.h"
#include "../peers/HubChatFrame.h"
#include "../peers/HubInfoPanel.h"
#include "../peers/UserCopyInfoMenu.h"

enum HubTaskType { 
	SUBSCRIPTIONS,
	UPDATE_USER_JOIN, 
#ifdef _DEBUG
	UPDATE_USER_LIST,
#endif
	UPDATE_USER, 
	REMOVE_USER, 
	ADD_CHAT_LINE,
	ADD_STATUS_LINE, 
	ADD_SILENT_STATUS_LINE, 
	SET_WINDOW_TITLE, 
	GET_PASSWORD, 
	PRIVATE_MESSAGE, 
	STATS, 
	CONNECTED, 
	DISCONNECTED, 
	CHEATING_USER,
	GET_SHUTDOWN, 
	SET_SHUTDOWN, 
	KICK_MSG
};

struct CompareItems;

class HubChatFrame;

class HubFrame : 
  public MDITabChildWindowImpl<HubFrame, IDI_HUB_NEW_ONLINE, IDI_HUB_NEW_OFFLINE>, 
  public WindowSortOrder,
  private ClientListener, 
#ifdef HUB_FRAME_SPLITTER
  public CSplitterImpl<HubFrame>, 
#endif
  private FavoriteManagerListener, 
  private TimerManagerListener,
  public UCHandler<HubFrame>, 
  private SettingsManagerListener,
  private UserListControlListener,
  private ChatControlListener,
  private HubInfoPanelController,
  private WindowListener,
  private UserInfoGetter
{
  friend class HubInfoPanel;
  friend class SubscriptionFrame;
public:
	DECLARE_FRAME_WND_CLASS_EX(_T("HubFrame"), IDI_HUB_NEW_ONLINE, 0, COLOR_3DFACE);

#ifdef HUB_FRAME_SPLITTER
	typedef CSplitterImpl<HubFrame> splitBase;
#endif
	typedef MDITabChildWindowImpl<HubFrame, IDI_HUB_NEW_ONLINE, IDI_HUB_NEW_OFFLINE> baseClass;
	typedef UCHandler<HubFrame> ucBase;

	BEGIN_MSG_MAP(HubFrame)
		MESSAGE_HANDLER(WM_CLOSE, onClose)
		MESSAGE_HANDLER(WM_SETFOCUS, onSetFocus)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SPEAKER, onSpeaker)
		MESSAGE_HANDLER(WM_CTLCOLORSTATIC, onCtlColor)
		MESSAGE_HANDLER(WM_CTLCOLOREDIT, onCtlColor)
		MESSAGE_HANDLER(FTM_CONTEXTMENU, onTabContextMenu)
		MESSAGE_HANDLER(WPM_APP_MINIMIZE, onAppMinimize)
		MESSAGE_HANDLER(WPM_MDI_CHILD_GETMINMAXINFO, onGetMinMaxInfo)
		
		COMMAND_ID_HANDLER(ID_FILE_RECONNECT, onFileReconnect)
		COMMAND_ID_HANDLER(IDC_REFRESH, onRefresh)
		COMMAND_ID_HANDLER(IDC_FOLLOW, onFollow)
		COMMAND_ID_HANDLER(IDC_ADD_AS_FAVORITE, onAddAsFavorite)
		COMMAND_ID_HANDLER(IDC_CLOSE_WINDOW, onCloseWindow)

		COMMAND_ID_HANDLER(IDC_SELECT_USER, onSelectUser)
		COMMAND_ID_HANDLER(IDC_PUBLIC_MESSAGE, onPublicMessage)
		COMMAND_ID_HANDLER(IDC_BAN_IP, onBanIP)
		COMMAND_ID_HANDLER(IDC_UNBAN_IP, onUnBanIP)
		COMMAND_ID_HANDLER(IDC_OPEN_HUB_LOG, onOpenHubLog)
		COMMAND_ID_HANDLER(IDC_OPEN_USER_LOG, onOpenUserLog)
//[-]PPA		COMMAND_ID_HANDLER(IDC_WHOIS_IP, onWhoisIP)
		COMMAND_ID_HANDLER(IDC_WINAMP_SPAM, onWinampSpam)
		COMMAND_ID_HANDLER(IDC_COPY_HUBNAME, onCopyHubInfo)
		COMMAND_ID_HANDLER(IDC_COPY_HUBADDRESS, onCopyHubInfo)
		COMMAND_ID_HANDLER(IDC_IGNORE, onIgnore)
		COMMAND_ID_HANDLER(IDC_UNIGNORE, onUnignore)
		if (m_chatControl) CHAIN_COMMANDS_MEMBER((*m_chatControl));
		CHAIN_COMMANDS_MEMBER(m_userListControl)
		CHAIN_COMMANDS(ucBase)
		CHAIN_MSG_MAP(baseClass)
#ifdef HUB_FRAME_SPLITTER
		CHAIN_MSG_MAP(splitBase)
#endif
	END_MSG_MAP()

	LRESULT OnForwardMsg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onCopyHubInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onTabContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onGetToolTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT onFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onRButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT onClientRButtonDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
	
	LRESULT onSelectUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onWhoIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onBanned(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
//[-]PPA	LRESULT onWhoisIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


	LRESULT onWinampSpam(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);


	void UpdateLayout(BOOL bResizeBars = TRUE);
        
	/* добавляет системное сообщение в чат */
	void addSystemMessage(const tstring& aLine);
	/* добавляет системное сообщение в чат или в нижнюю панель */
	void addMessage(bool inChat, const tstring& aLine);
	/* добавляет системное сообщение в нижнюю панель */
	void addStatusMessage(const tstring& aLine, const tstring& author);
	/* добавляет системное сообщение в нижнюю панель */
	void addStatusMessage(const tstring& aLine);
	/* добавляет пользовательское сообщение в чат, если он открыт */
	void addChatMessage(const Identity& i, const tstring& aLine, CHARFORMAT2& cf);

	void handleTab(bool reverse);
	void runUserCommand(::UserCommand& uc);

#ifdef HUB_FRAME_SPLITTER
        bool SetSplitterPos(int xyPos = -1, bool bUpdate = true);
#endif
private:
	friend class HubFrameFactory;
	static void openWindow(const tstring& server
		, bool autoConnect
		, const tstring& rawOne = Util::emptyStringT
		, const tstring& rawTwo = Util::emptyStringT
		, const tstring& rawThree = Util::emptyStringT
		, const tstring& rawFour = Util::emptyStringT
		, const tstring& rawFive = Util::emptyStringT
		, int windowposx = 0, int windowposy = 0, int windowsizex = 0, int windowsizey = 0, int windowtype = 0, int chatusersplit = 0, bool userliststate = true,
		        string sColumsOrder = Util::emptyString, string sColumsWidth = Util::emptyString, string sColumsVisible = Util::emptyString);
	static void resortUsers();
	static void closeDisconnected();
	static void reconnectDisconnected();
	static void closeAll(size_t thershold = 0);
public:
	void activateChat(bool activateIfAlreadyCreated);

	static HubFrame* getHub(Client* aClient);

	LRESULT onSetFocus(UINT /* uMsg */, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
		m_userListControl.SetFocus();
		return 0;
	}

	LRESULT onAddAsFavorite(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		addAsFavorite(false);
		return 0;
	}

	LRESULT onCloseWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		PostMessage(WM_CLOSE);
		return 0;
	}

	LRESULT onRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
		if(client->isConnected()) {
			clearUserList();
			client->refreshUserList(false);
		}
		return 0;
	}

	

	LRESULT onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/);

	LRESULT onAppMinimize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { m_infoPanel.destroyBanner(); return 0; }
	LRESULT onGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
private:
	HubFrame(const tstring& aServer
		, bool autoConnect
		, const tstring& aRawOne
		, const tstring& aRawTwo
		, const tstring& aRawThree
		, const tstring& aRawFour
		, const tstring& aRawFive
		, int chatusersplit, bool userliststate);

	virtual ~HubFrame();

	typedef HASH_MAP<tstring, HubFrame*> FrameMap;
	typedef FrameMap::const_iterator FrameIter;
	static FrameMap frames;

	/* адрес, на который нас послали с этого хаба */
	tstring redirect;
	bool showJoins;
	bool favShowJoins;
	bool exclChecks;
	tstring opChat;

	bool waitingForPW;

	Client* client;
	bool m_autoConnect;
	int m_connectedCount;
	bool m_chatWasClosed;
	tstring server;
	tstring hubName;

	UserCopyInfoMenu copyMenu;
	OMenu copyHubMenu;
	OMenu userMenu;
	OMenu tabMenu;

	HubInfoPanel m_infoPanel;
	HubInfoNumberPanel m_numberPanel;
	ChatControl* m_chatControl;
	UserListControl m_userListControl;
	vector<UserListControl*> m_userLists;
	HubChatFrame* m_chatFrame;
	vector<tstring> lastMessages;

	bool isWrongNick();

	typedef void (HubFrame::*HubCommandMethod)(ChatCommandContext*);
	typedef map<string, HubCommandMethod> HubCommandMap;
	static HubCommandMap m_commands;
	static void initCommandMap();
	void commandJoin(ChatCommandContext* context);
	void commandToggleShowJoins(ChatCommandContext*);
	void commandToggleFavShowJoins(ChatCommandContext*);
	void commandClose(ChatCommandContext*);
	void commandConnection(ChatCommandContext*);
	void commandGetFileList(ChatCommandContext* context);
	void commandLog(ChatCommandContext* context);
	void commandExtraSlots(ChatCommandContext* context);
	void commandSmallFileSize(ChatCommandContext* context);
	void commandSaveQueue(ChatCommandContext*);
	void commandIgnoreList(ChatCommandContext*);
	void commandAddAsFavorite(ChatCommandContext*);
	void commandRemoveFavorite(ChatCommandContext*);
	void commandHelp(ChatCommandContext*);
	void commandPassword(ChatCommandContext*);
	void commandPrivateMessage(ChatCommandContext* context);
	void commandStats(ChatCommandContext*);
	void commandPubStats(ChatCommandContext*);
	void commandMe(ChatCommandContext* context);
	void commandTestPDB(ChatCommandContext* context);


	bool tabMenuShown;

	UserInfo::UserMap userMap;
	TaskQueue tasks;
	bool updateUsers;
	bool resort;

	StringMap ucLineParams;

	bool updateUser(const UserTask& u);
	void removeUser(const UserPtr& aUser);

	bool isCheatingUser(const Identity& identity) const;
	void onCheatingUser(const UserPtr& user, const Identity& identity);

	UserInfo* findUser(const tstring& nick) { // !SMT!-S
		return userMap.findUser(ClientManager::getInstance()->getUser(Text::fromT(nick), client->getHubUrl()));
	}
	UserPtr& getSelectedUserPtr() { return m_userListControl.getSelectedUserPtr(); }
	UserInfo* getSelectedUser() { return findUser(m_userListControl.getSelectedUserPtr()); }
	UserInfo* findUser(const UserPtr& user) { return userMap.findUser(user); }

	void addAsFavorite(bool inChat);
	void removeFavoriteHub(bool inChat);

	void clearUserList();
	void clearTaskList();

	int hubchatusersplit;

	bool PreparePopupMenu(bool bIsChat, const UserPtr& aUser, OMenu *pMenu);

	void updateStatusBar() { if(m_hWnd) speak(STATS); }
	string getHubName() const throw();

	// FavoriteManagerListener
	virtual void on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) throw();
	virtual void on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) throw();
	void resortForFavsFirst(bool justDoIt = false);

	// TimerManagerListener
	virtual void on(TimerManagerListener::Second, uint32_t /*aTick*/) throw();
	virtual void on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw();

	// ClientListener
	virtual void on(Connecting, Client*) throw();
	virtual void on(Connected, Client*) throw();
	virtual void on(UserIdentified, Client*) throw();
	virtual void on(UserUpdated, Client*, const UserPtr&) throw(); // !SMT!-fix
	virtual void on(UsersUpdated, Client*, const User::PtrList&) throw();
	virtual void on(ClientListener::UserRemoved, Client*, const UserPtr&) throw();
	virtual void on(Redirect, Client*, const string&) throw();
	virtual void on(Failed, Client*, const string&) throw();
	virtual void on(GetPassword, Client*) throw();
	virtual void on(HubUpdated, Client*) throw();
	virtual void on(Message, Client*, const UserPtr&, const string&) throw();
	virtual void on(PrivateMessage, Client*, const UserPtr&, const UserPtr, const UserPtr, const string&, bool = true) throw(); // !SMT!-S
	virtual void on(NickTaken, Client*) throw();
	virtual void on(SearchFlood, Client*, const string&) throw();
	virtual void on(CheatMessage, Client*, const string&) throw();	
	virtual void on(HubTopic, Client*, const string&) throw();
#ifdef _DEBUG
	virtual void on(UserListReceived, Client*) throw() { speak(UPDATE_USER_LIST); }
#endif

	void speak(HubTaskType s) { tasks.add(static_cast<uint8_t>(s), 0); PostMessage(WM_SPEAKER); }
	void speak(HubTaskType s, const string& msg) { tasks.add(static_cast<uint8_t>(s), new StringTask(msg)); PostMessage(WM_SPEAKER); }

	// !SMT!-fix
	void speak(HubTaskType s, const UserPtr& u) {
		tasks.add(static_cast<uint8_t>(s), new UserTask(u, client));
		updateUsers = true;
	}

	void speak(HubTaskType s, const UserPtr& from, const UserPtr& to, const UserPtr& replyTo, const string& line, bool annoying = true) {
		tasks.add(static_cast<uint8_t>(s), new MessageTask(from, to, replyTo, line, annoying));
		PostMessage(WM_SPEAKER);
	} 
	// !SMT!-S

	virtual bool isLargeIcon() const { return true; }

	virtual void onActivate() { m_infoPanel.createBanner(); }
	virtual void onDeactivate() { m_infoPanel.destroyBanner(); }

	// UserListControlListener
	virtual void onDoubleClickUser(UserListControl* control, UserInfo* ui);
	virtual bool buildUserListMenu(UserPtr& aSelectedUser, OMenu* menu);
	virtual void cleanUserListMenu(OMenu* menu);
	virtual void userListMessage(const tstring& line);
	virtual UserInfo::UserMap& getUserMap() { return userMap; }
	virtual Client* getClient() { return client; }

	// ChatControl
	virtual bool chatExecuteCommand(ChatCommandContext* context);
	virtual void chatSendMessage(const tstring& s);
	virtual void onUserDblClick(UserInfo* ui, WPARAM wParam);
	virtual bool onUserContextMenu(UserInfo* ui, CPoint pt);
	virtual void autoCompleteFaliure();
	virtual void autoCompleteBegin();
	virtual tstring autoCompleteUserNick(const tstring& prefix);

	// WindowSortOrder
	virtual WindowSortOrders getSortOrder();

	//HubInfoPanelController
	//virtual Client* getClient() - already defined

	// WindowListener
	virtual void windowClosed(MDIContainer::Window window);

	void putNickToMessageEdit(UserInfo* ui);

	void buildChatMenu(OMenu *pMenu);
	void buildUserMenu(OMenu *pMenu, const UserPtr& aUser, bool bIsChat);

private:
        static void addDupeUsersToSummaryMenu(const int64_t &share, const string& ip); // !SMT!-UI
};

#endif // !defined(HUB_FRAME_H)

/**
* @file
* $Id: HubFrame.h,v 1.43.2.5 2008/12/21 14:29:37 alexey Exp $
*/
