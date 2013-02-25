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

#include "HubFrame.h"
#include "LineDlg.h"
#include "../peers/PrivateFrameFactory.h"
#include "TextFrame.h"
#include "ChatBot.h"

#include "../client/QueueManager.h"
#include "../client/ShareManager.h"
#include "../client/FavoriteManager.h"
#include "../client/LogManager.h"
#include "../client/AdcCommand.h"
#include "../client/ConnectionManager.h" 
#include "../client/NmdcHub.h"
#include "../client/Wildcards.h"
#include "../client/IgnoreManager.h"

#include "../client/pme.h"
#include "../peers/PeersUtils.h"
#include "../peers/PeersVersion.h"
#include "../peers/PeersUtils.h"
#include "../peers/HubMessageControl.h"
#include "../peers/SubscriptionFrame.h"
#include "../peers/Sounds.h"

#include "../client/CID.h"

#define VPADDING 8

HubFrame::FrameMap HubFrame::frames;

HubFrame::HubFrame(const tstring& aServer,
				   bool autoConnect,
                   const tstring& aRawOne, 
                   const tstring& aRawTwo,
                   const tstring& aRawThree,
                   const tstring& aRawFour,
                   const tstring& aRawFive,
                   int chatusersplit, 
                   bool userliststate) : 
waitingForPW(false), 
server(aServer), 
hubName(aServer),
m_autoConnect(autoConnect),
m_connectedCount(0),
m_chatWasClosed(false),
updateUsers(false),
resort(false),
hubchatusersplit(chatusersplit),
copyMenu(this),
m_infoPanel(this),
m_numberPanel(this),
m_chatFrame(NULL),
m_chatControl(NULL),
m_userListControl(this)
{
  client = ClientManager::getInstance()->getClient(Text::fromT(aServer));
  client->addListener(this);

  client->setRawOne(Text::fromT(aRawOne));
  client->setRawTwo(Text::fromT(aRawTwo));
  client->setRawThree(Text::fromT(aRawThree));
  client->setRawFour(Text::fromT(aRawFour));
  client->setRawFive(Text::fromT(aRawFive));

  initCommandMap();
}

HubFrame::~HubFrame() {
  ClientManager::getInstance()->putClient(client);

  dcassert(frames.find(server) != frames.end());
  dcassert(frames[server] == this);
  frames.erase(server);

  clearTaskList();
  dcassert(userMap.empty());
}

LRESULT HubFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
        m_infoPanel.Create(m_hWnd);
		m_numberPanel.Create(m_hWnd);
        m_userListControl.Create(m_hWnd);
        m_userLists.push_back(&m_userListControl);
#ifdef HUB_FRAME_SPLITTER
	SetSplitterPanes(m_infoPanel.m_hWnd, m_userListControl.m_hWnd, false);
        m_nProportionalPos = 5000;
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);


	if(hubchatusersplit)
		m_nProportionalPos = hubchatusersplit;
#endif

        m_userListControl.initColumns(server);

		::ShowWindow(m_userListControl.m_hWnd, SW_HIDE);

	copyHubMenu.CreatePopupMenu();
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBNAME, CTSTRING(HUB_NAME));
	copyHubMenu.AppendMenu(MF_STRING, IDC_COPY_HUBADDRESS, CTSTRING(HUB_ADDRESS));

	tabMenu.CreatePopupMenu();
	if(BOOLSETTING(LOG_MAIN_CHAT)) {
		tabMenu.AppendMenu(MF_STRING, IDC_OPEN_HUB_LOG, CTSTRING(OPEN_HUB_LOG));
		tabMenu.AppendMenu(MF_SEPARATOR);
	}
	tabMenu.AppendMenu(MF_STRING, IDC_ADD_AS_FAVORITE, CTSTRING(ADD_TO_FAVORITES));
	tabMenu.AppendMenu(MF_STRING, ID_FILE_RECONNECT, CTSTRING(MENU_RECONNECT));
	tabMenu.AppendMenu(MF_POPUP, (UINT)(HMENU)copyHubMenu, CTSTRING(COPY));
	tabMenu.AppendMenu(MF_SEPARATOR);	
	tabMenu.AppendMenu(MF_STRING, IDC_CLOSE_WINDOW, CTSTRING(CLOSE));

        const FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
	showJoins = (fhe != NULL && fhe->getShowJoins()) || BOOLSETTING(SHOW_JOINS);
	favShowJoins = BOOLSETTING(FAV_SHOW_JOINS);
        opChat = fhe ? Text::toT(fhe->getOpChat()) : Util::emptyStringT;
	exclChecks = fhe !=NULL && fhe->getExclChecks();

	bHandled = FALSE;
	client->connect();

	FavoriteManager::getInstance()->addListener(this);
        TimerManager::getInstance()->addListener(this);
	SettingsManager::getInstance()->addListener(this);

	return 1;
}

void HubFrame::openWindow(const tstring& aServer
						    , bool autoConnect
							, const tstring& rawOne /*= Util::emptyString*/
							, const tstring& rawTwo /*= Util::emptyString*/
							, const tstring& rawThree /*= Util::emptyString*/
							, const tstring& rawFour /*= Util::emptyString*/
							, const tstring& rawFive /*= Util::emptyString*/
		, int windowposx, int windowposy, int windowsizex, int windowsizey, int windowtype, int chatusersplit, bool userliststate,
       string sColumsOrder, string sColumsWidth, string sColumsVisible) {
	FrameIter i = frames.find(aServer);
	if(i == frames.end()) {
		HubFrame* frm = new HubFrame(aServer
			, autoConnect
			, rawOne
			, rawTwo 
			, rawThree 
			, rawFour 
			, rawFive 
			, chatusersplit, userliststate);
		frames[aServer] = frm;

		int nCmdShow = SW_SHOWDEFAULT;
		CRect rc = frm->rcDefault;

		rc.left = windowposx;
		rc.top = windowposy;
		rc.right = rc.left + windowsizex;
		rc.bottom = rc.top + windowsizey;
		if( (rc.left < 0 ) || (rc.top < 0) || (rc.right - rc.left < 10) || ((rc.bottom - rc.top) < 10) ) {
			rc = frm->rcDefault;
		}
		frm->CreateEx(WinUtil::mdiClient, rc);
		if(BOOLSETTING(HUB_SMALL))
			frm->ShowWindow(SW_MINIMIZE);
		else if(windowtype)
			frm->ShowWindow(((nCmdShow == SW_SHOWDEFAULT) || (nCmdShow == SW_SHOWNORMAL)) ? windowtype : nCmdShow);
	} else {
		if(::IsIconic(i->second->m_hWnd))
			::ShowWindow(i->second->m_hWnd, SW_RESTORE);
		i->second->MDIActivate(i->second->m_hWnd);
	}
}

HubFrame::HubCommandMap HubFrame::m_commands;

void HubFrame::initCommandMap() {
  if (!m_commands.empty()) {
    return;
  }
  m_commands["join"] = &HubFrame::commandJoin;
  m_commands["showjoins"] = &HubFrame::commandToggleShowJoins;
  m_commands["favshowjoins"] = &HubFrame::commandToggleFavShowJoins;
  m_commands["close"] = &HubFrame::commandClose;
  m_commands["connection"] = &HubFrame::commandConnection;
  m_commands["getlist"] = &HubFrame::commandGetFileList;
  m_commands["log"] = &HubFrame::commandLog;
  m_commands["extraslots"] = &HubFrame::commandExtraSlots;
  m_commands["smallfilesize"] = &HubFrame::commandSmallFileSize;
  m_commands["savequeue"] = &HubFrame::commandSaveQueue;
  m_commands["ignorelist"] = &HubFrame::commandIgnoreList;
  m_commands["favorite"] = &HubFrame::commandAddAsFavorite;
  m_commands["fav"] = &HubFrame::commandAddAsFavorite;
  m_commands["removefavorite"] = &HubFrame::commandRemoveFavorite;
  m_commands["removefav"] = &HubFrame::commandRemoveFavorite;
  m_commands["help"] = &HubFrame::commandHelp;
  m_commands["h"] = &HubFrame::commandHelp;
  m_commands["password"] = &HubFrame::commandPassword;
  m_commands["pm"] = &HubFrame::commandPrivateMessage;
  m_commands["stats"] = &HubFrame::commandStats;
  m_commands["pubstats"] = &HubFrame::commandPubStats;
  m_commands["me"] = &HubFrame::commandMe;
  m_commands["test-pdb"] = &HubFrame::commandTestPDB;
}

void HubFrame::commandTestPDB(ChatCommandContext* /*context*/) {
	*((int*)NULL) = 0;
}

void HubFrame::commandJoin(ChatCommandContext* context) {
  if (!context->param.empty()) {
    if (BOOLSETTING(JOIN_OPEN_NEW_WINDOW)) {
      HubFrame::openWindow(context->param, false);
    } 
    else {
      redirect = context->param;
      PostMessage(WM_COMMAND, IDC_FOLLOW);
    }
  } 
  else {
    addSystemMessage(TSTRING(SPECIFY_SERVER));
  }
}

void HubFrame::commandToggleShowJoins(ChatCommandContext*) {
  showJoins = !showJoins;
  addSystemMessage(showJoins ? TSTRING(JOIN_SHOWING_ON) : TSTRING(JOIN_SHOWING_OFF));
}

void HubFrame::commandToggleFavShowJoins(ChatCommandContext*) {
  favShowJoins = !favShowJoins;
  addSystemMessage(favShowJoins ? TSTRING(FAV_JOIN_SHOWING_ON) : TSTRING(FAV_JOIN_SHOWING_OFF));
}

void HubFrame::commandClose(ChatCommandContext*) {
  PostMessage(WM_CLOSE);
}

void HubFrame::commandConnection(ChatCommandContext*) {
  const ConnectionManager* connM = ConnectionManager::getInstance();
  const SearchManager* searchM = SearchManager::getInstance();
  const string msg = STRING(IP) + client->getLocalIp() + ", " + 
    STRING(PORT) + 
    Util::toString(connM->getPort()) + "/" + 
    Util::toString(searchM->getPort()) + "/" +
    Util::toString(connM->getSecurePort());
  addSystemMessage(Text::toT(msg));
}

void HubFrame::commandGetFileList(ChatCommandContext* context) {
  if (!context->param.empty() ){
    UserInfo* ui = findUser(context->param);
    if (ui) {
      ui->getList();
    }
  }
}

void HubFrame::commandLog(ChatCommandContext* context) {
  StringMap params;
  params["hubNI"] = client->getHubName();
  params["hubURL"] = client->getHubUrl();
  params["myNI"] = client->getMyNick(); 
  if (context->param.empty()) {
    WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params, false))));
  } 
  else if(Util::stricmp(context->param.c_str(), _T("status")) == 0) {
    WinUtil::openFile(Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_STATUS), params, false))));
  }
}

void HubFrame::commandExtraSlots(ChatCommandContext* context) {
  int j = Util::toInt(Text::fromT(context->param));
  if (j > 0) {
    SettingsManager::getInstance()->set(SettingsManager::EXTRA_SLOTS, j);
    addSystemMessage(TSTRING(EXTRA_SLOTS_SET));
  } 
  else {
    addSystemMessage(TSTRING(INVALID_NUMBER_OF_SLOTS));
  }
}

void HubFrame::commandSmallFileSize(ChatCommandContext* context) {
  int j = Util::toInt(Text::fromT(context->param));
  if (j >= 64) {
    SettingsManager::getInstance()->set(SettingsManager::SET_MINISLOT_SIZE, j);
    addSystemMessage(TSTRING(SMALL_FILE_SIZE_SET));
  } 
  else {
    addSystemMessage(TSTRING(INVALID_SIZE));
  }
}

void HubFrame::commandSaveQueue(ChatCommandContext*) {
  QueueManager::getInstance()->saveQueue();
  addSystemMessage(TSTRING(QUEUE_SAVED));
}

void HubFrame::commandIgnoreList(ChatCommandContext*) {
  tstring ignorelist = TSTRING(IGNORED_USERS) + _T(":");
  WStringList users;
  IgnoreManager::getInstance()->getIgnoredUsers(users);
  for (WStringList::const_iterator i = users.begin(); i != users.end(); ++i) {
    ignorelist += _T(" ") + *i;
  }
  addSystemMessage(ignorelist);
}

void HubFrame::commandAddAsFavorite(ChatCommandContext*) {
  addAsFavorite(true);
}

void HubFrame::commandRemoveFavorite(ChatCommandContext*) {
  removeFavoriteHub(true);
}

void HubFrame::commandHelp(ChatCommandContext*) {
  addSystemMessage(WinUtil::getChatHelp());
}

void HubFrame::commandPassword(ChatCommandContext* context) {
  if (waitingForPW) {
    client->setPassword(Text::fromT(context->param));
    client->password(Text::fromT(context->param));
    waitingForPW = false;
  }
}

void HubFrame::commandPrivateMessage(ChatCommandContext* context) {
  string::size_type j = context->param.find(_T(' '));
  if (j != string::npos) {
    tstring nick = context->param.substr(0, j);
    UserInfo* ui = findUser(nick);
    if (ui) {
      if (context->param.size() > j + 1) {
        PrivateFrameFactory::openWindow(ui->getUser(), context->param.substr(j+1));
      }
      else {
        PrivateFrameFactory::openWindow(ui->getUser());
      }
    }
  } 
  else if (!context->param.empty()) {
    UserInfo* ui = findUser(context->param);
    if (ui) {
      PrivateFrameFactory::openWindow(ui->getUser());
    }
  }
}

void HubFrame::commandStats(ChatCommandContext*) {
  addSystemMessage(Text::toT(WinUtil::generateStats())); // TODO level = INFO
}

void HubFrame::commandPubStats(ChatCommandContext*) {
  client->hubMessage(WinUtil::generateStats());
}

void HubFrame::commandMe(ChatCommandContext* context) {
  client->hubMessage(Text::fromT(context->getCommandLine()));
}

void HubFrame::activateChat(bool activateIfAlreadyCreated) {
  if (m_chatFrame == NULL) {
    m_chatFrame = new HubChatFrame(this, this, this);
    m_chatFrame->addWindowListener(this);
    m_chatFrame->CreateEx(WinUtil::mdiClient, rcDefault);
    m_chatFrame->SetWindowText((TSTRING(PEERS_TOOLBAR_CHAT) + _T(" ") + hubName).c_str());
    m_chatFrame->ShowWindow(SW_MAXIMIZE);
    m_chatControl = &m_chatFrame->m_chatControl;
    m_userLists.push_back(&m_chatFrame->m_userListControl);
  }
  else if (activateIfAlreadyCreated) {
    WinUtil::MDIActivate(m_chatFrame->m_hWnd);
  }
}

void HubFrame::windowClosed(MDIContainer::Window window) {
  if (window == m_chatFrame) {
    vector<UserListControl*>::iterator i = find(m_userLists.begin(), m_userLists.end(), &m_chatFrame->m_userListControl);
    dcassert(i != m_userLists.end());
    m_userLists.erase(i);
    m_chatFrame = NULL;
    m_chatControl = NULL;
	m_chatWasClosed = true;
  }
}

bool HubFrame::chatExecuteCommand(ChatCommandContext* context) {
  HubCommandMap::iterator i = m_commands.find(Text::fromT(context->command));
  if (i != m_commands.end()) {
    (this->*(i->second))(context);
  }
  else if (waitingForPW) {
    addSystemMessage(TSTRING(DONT_REMOVE_SLASH_PASSWORD));
    if (!m_chatControl) {
      activateChat(true);
    }
    m_chatControl->setMessageText(_T("/password "));
  }
  else {
    tstring message;
    tstring status;
    if (WinUtil::checkCommand(context->command, context->param, message, status)) {
      if (!message.empty()) {
        client->hubMessage(Text::fromT(message));
      }
      if (!status.empty()) {
        addSystemMessage(status);
      }
    }
    else {
      if (BOOLSETTING(SEND_UNKNOWN_COMMANDS)) {
        client->hubMessage(Text::fromT(context->getCommandLine()));
      } 
      else {
        return false;
      }
    }
  }
  return true;
}

void HubFrame::chatSendMessage(const tstring& s) {
  client->hubMessage(Text::fromT(s));
}

void HubFrame::addAsFavorite(bool inChat) {
  FavoriteHubEntry* existingHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
  if (!existingHub) {
    FavoriteHubEntry aEntry;
    aEntry.setServer(Text::fromT(server));
    aEntry.setName(client->getHubName());
    aEntry.setDescription(client->getHubDescription());
    aEntry.setConnect(false);
    aEntry.setNick(client->getMyNick());
    aEntry.setPassword(client->getPassword());
    FavoriteManager::getInstance()->addFavorite(aEntry);
    addMessage(inChat, TSTRING(FAVORITE_HUB_ADDED));
  }
  else {
    addMessage(inChat, TSTRING(FAVORITE_HUB_ALREADY_EXISTS));
  }
}

void HubFrame::removeFavoriteHub(bool inChat) {
  FavoriteHubEntry* removeHub = FavoriteManager::getInstance()->getFavoriteHubEntry(client->getHubUrl());
  if (removeHub) {
    FavoriteManager::getInstance()->removeFavorite(removeHub);
    addMessage(inChat, TSTRING(FAVORITE_HUB_REMOVED));
  }
  else {
    addMessage(inChat, TSTRING(FAVORITE_HUB_DOES_NOT_EXIST));
  }
}

LRESULT HubFrame::onCopyHubInfo(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if(client->isConnected()) {
        string sCopy;

		switch (wID) {
			case IDC_COPY_HUBNAME:
				sCopy += client->getHubName();
				break;
			case IDC_COPY_HUBADDRESS:
				sCopy += client->getHubUrl();
				break;
		}

		if (!sCopy.empty())
			WinUtil::setClipboard(Text::toT(sCopy));
    }
	return 0;
}

bool HubFrame::updateUser(const UserTask& u) {
  if (u.identity.getUser() != u.user) return false; // !SMT!-fix user offline

  UserInfo::UserMapIter i = userMap.find(u.user);
  if (i == userMap.end()) {
    UserInfo* ui = new UserInfo(u);
    userMap.insert(make_pair(u.user, ui));

    client->availableBytes += ui->getIdentity().getBytesShared();
    return true;
  } 
  else {
    UserInfo* ui = i->second;
    client->availableBytes -= ui->getIdentity().getBytesShared();
    client->availableBytes += u.identity.getBytesShared();
	ui->update(u.identity);
    return false;
  }
}

void HubFrame::removeUser(const UserPtr& aUser) {
  UserInfo::UserMap::iterator i = userMap.find(aUser);
  if (i == userMap.end()) {
    // Should never happen?

    // I removed the assertion because my memory saving feature can reache the code
    // One big idiot removed this code too, but he doesn't use the memory saving
    // feature, so he doesn't really know why the assertion should be there
    // dcassert(i != userMap.end());
    return;
  }
  UserInfo* ui = i->second;
  
  client->availableBytes -= ui->getIdentity().getBytesShared();
  userMap.erase(i);
  delete ui;
}

/* проверка подозрительно выглядещего размера шары */
bool HubFrame::isCheatingUser(const Identity& identity) const {
  const int64_t bytesSharedInt64 = identity.getBytesShared();
  if (bytesSharedInt64 > 0) {
    const string bytesShared = Util::toString(bytesSharedInt64);
    static const char* sSameNumbers[] = { "000000", "111111", "222222", "333333", "444444", "555555", "666666", "777777", "888888", "999999" };
    for (int i = 0; i < 10; ++i) {
      if (strstr(bytesShared.c_str(), sSameNumbers[i]) != NULL) {
        return true;
      }
    }
  }
  return false;
}

/* нотификация о том, что у пользователя размер шары подозрительный */
void HubFrame::onCheatingUser(const UserPtr& user, const Identity& identity) {
  string detectString = Text::fromT(Util::formatExactSize(identity.getBytesShared()))+" - the share size had too many same numbers in it";
  ClientManager::getInstance()->setFakeList(user, detectString);
  char buf[512];
  snprintf(buf, sizeof(buf), "*** %s %s - %s", STRING(USER).c_str(), identity.getNick().c_str(), detectString.c_str());
  if (BOOLSETTING(POPUP_CHEATING_USER)) {
    WinUtil::ShowBalloonTip(Text::toT(buf).c_str(), CTSTRING(CHEATING_USER));
  }
  addStatusMessage(Text::toT(buf));
  identity.sendRawCommand(*client, SETTING(FAKESHARE_RAW));
}

LRESULT HubFrame::onSpeaker(UINT /*uMsg*/, WPARAM /* wParam */, LPARAM /* lParam */, BOOL& /*bHandled*/) {
#ifdef _DEBUG
	UINT startTime = GetTickCount();
#endif
  TaskQueue::List t;
  tasks.get(t);
  if (t.empty()) {
	  return 0;
  }

  for (vector<UserListControl*>::iterator i = m_userLists.begin(); i != m_userLists.end(); ++i) {
	  (*i)->beginUpdate();
  }

  for(TaskQueue::Iter i = t.begin(); i != t.end(); ++i) {
	  if (i->first == SUBSCRIPTIONS) {
		  if (client->getState() == Client::STATE_NORMAL) {
			  if (SubscriptionFrame::getInstance() == NULL) {
				  HWND wasActive = WinUtil::MDIGetActive();
				  // TODO use constructor & destructor
				  ::ShowWindow(m_hWndMDIClient, SW_HIDE);
				  SubscriptionFrame::openWindow(false);
				  if (WinUtil::MDIGetActive() != wasActive) {
					  ::SendMessage(m_hWndMDIClient, WM_MDIACTIVATE, (WPARAM)wasActive, 0);
				  }
				  ::ShowWindow(m_hWndMDIClient, SW_SHOW);
			  }
		  }
	  }
#ifdef _DEBUG
	  else if (i->first == UPDATE_USER_LIST) {
		  for (vector<UserListControl*>::iterator j = m_userLists.begin(); j != m_userLists.end(); ++j) {
			  (*j)->reloadUserList();
		  }
	  }
#endif
	  else if(i->first == UPDATE_USER) {
		  UserTask& u = *static_cast<UserTask*>(i->second);
		  //dcdebug("UPDATE_USER %s\n", u.user->getFirstNick().c_str());
		  updateUser(u);
	  } 
	  else if(i->first == UPDATE_USER_JOIN) {
		  UserTask& u = *static_cast<UserTask*>(i->second);
		  //dcdebug("UPDATE_USER_JOIN %s\n", u.user->getFirstNick().c_str());
		  if(updateUser(u)) {
			  bool isFavorite = FavoriteManager::getInstance()->isFavoriteUser(u.user);
			  // !SMT!-S
			  if (isFavorite && FavoriteManager::getInstance()->getFavoriteUsers().find(u.user->getCID())->second.getUploadLimit() == FavoriteUser::UL_BAN) {
				  isFavorite = false;
			  }
			  if (isFavorite) {
				  Sounds::PlaySound(SettingsManager::SOUND_FAVUSER);
			  }
			  if(isFavorite && BOOLSETTING(POPUP_FAVORITE_CONNECTED)) {
				  WinUtil::ShowBalloonTip(Text::toT(u.identity.getNick() + " - " + client->getHubName()).c_str(), CTSTRING(FAVUSER_ONLINE));
			  }
			  if (showJoins || (favShowJoins && isFavorite)) {
				  addStatusMessage(TSTRING(JOINS) + Text::toT(u.identity.getNick()));
			  }
			  if (client->isOp() && !u.identity.isBot() && !u.identity.isHub()) {
				  if (isCheatingUser(u.identity)) {
					  onCheatingUser(u.user, u.identity);
				  }
				  if (BOOLSETTING(CHECK_NEW_USERS) && (exclChecks == false) && (Wildcard::patternMatch(u.identity.getNick(), SETTING(PROT_USERS), ';', false) == false)) {
					  if (!BOOLSETTING(PROT_FAVS) || !isFavorite) { // !SMT!-opt
						  if(u.identity.isTcpActive() || client->isActive()) {
							  try {
								  QueueManager::getInstance()->addTestSUR(u.user, true);
							  } catch(const Exception&) {
								  //...
							  }
						  }
					  }
				  }
			  }
		  }
	  } 
	  else if (i->first == REMOVE_USER) {
		  UserTask& u = *static_cast<UserTask*>(i->second);
		  //dcdebug("REMOVE_USER %s\n", u.user->getFirstNick().c_str());
		  // !SMT!-S !SMT!-UI
		  bool isFavorite = FavoriteManager::getInstance()->isFavoriteUser(u.user);
		  if (isFavorite && FavoriteManager::getInstance()->getFavoriteUsers().find(u.user->getCID())->second.getUploadLimit() == FavoriteUser::UL_BAN) isFavorite = false;
		  if (isFavorite) {
			  Sounds::PlaySound(SettingsManager::SOUND_FAVUSER_OFFLINE);
		  }
		  if (isFavorite && BOOLSETTING(POPUP_FAVORITE_DISCONNECTED)) {
			  WinUtil::ShowBalloonTip(Text::toT(u.user->getFirstNick() + " - " + client->getHubName()).c_str(), CTSTRING(FAVUSER_OFFLINE));
		  }
		  removeUser(u.user);
		  if (showJoins || (favShowJoins && isFavorite)) {
			  addStatusMessage(TSTRING(PARTS) + Text::toT(u.user->getFirstNick())); // !SMT!-fix
		  }
	  } 
	  else if (i->first == CONNECTED) {
		  addStatusMessage(TSTRING(CONNECTED));
		  unsetIconState();
		  if (m_chatFrame) {
			  m_chatFrame->unsetIconState();
		  }
		  if (BOOLSETTING(POPUP_HUB_CONNECTED)) {
			  WinUtil::ShowBalloonTip(Text::toT(client->getAddress()).c_str(), CTSTRING(CONNECTED));
		  }
		  Sounds::PlaySound(SettingsManager::SOUND_HUBCON);
		  if (!ClientManager::getInstance()->isActive(client->getHubUrl())) {
			  addStatusMessage(TSTRING(PASSIVE_NOTICE));
		  }
		  /* ignoring open chat setting
		  if (BOOLSETTING(OPEN_HUB_CHAT_ON_CONNECT) && !m_chatWasClosed) {
			  activateChat(false);
			  if (!(m_autoConnect && ++m_connectedCount == 1)) {
				  if (WinUtil::MDIGetActive() != m_hWnd) {
					  ::PostMessage(m_hWndMDIClient, WM_MDIACTIVATE, (WPARAM)m_hWnd, 0);
				  }
			  }
		  }
		  */
	  } 
	  else if (i->first == DISCONNECTED) {
		  clearUserList();
		  setIconState();
		  if (m_chatFrame) {
			  m_chatFrame->setIconState();
		  }
		  Sounds::PlaySound(SettingsManager::SOUND_HUBDISCON);
		  if(BOOLSETTING(POPUP_HUB_DISCONNECTED)) {
			  WinUtil::ShowBalloonTip(Text::toT(client->getAddress()).c_str(), CTSTRING(DISCONNECTED));
		  }
		  if (BOOLSETTING(NICK_ADD_UNIQUE_SUFFIX) && isWrongNick()) {
			  // generate a chaos randomly nick as CID!
			  // Fix #PEERS1-58
			  string nick = CID::generate().toBase32();
			  SettingsManager::getInstance()->set(SettingsManager::NICK, nick);
			  m_infoPanel.updateData();
			  client->reconnect();
		  }
	  } 
	  else if(i->first == ADD_CHAT_LINE) {
		  // !SMT!-S
		  MessageTask& msg = *static_cast<MessageTask*>(i->second);
		  if (msg.from && (msg.from->isSet(User::BOT) 
			  || msg.fromId.isHub()
			  || msg.from->getCountSlots() == 0 && msg.from->getBytesShared() == 0 && userMap.size() <= 2)) 
		  {
			  // второе условие - информация о версии хаба (первый пользователь наш, второй - хаб).
#ifdef _DEBUG
			  addStatusMessage(Text::toT(msg.str + ":" + Util::toString(msg.fromId.getFlags())), Text::toT(msg.from->getFirstNick()));
#else
			  addStatusMessage(Text::toT(msg.str), Text::toT(msg.from->getFirstNick()));
#endif
		  }
		  else {
			  bool fromHub = msg.fromId.isOp() || msg.fromId.isHub();
			  bool hubSecure = fromHub && ((msg.str.find("Hub-Security") != string::npos) && (msg.str.find("was kicked by") != string::npos));
			  bool kick = fromHub && !hubSecure && ((msg.str.find("is kicking") != string::npos) && (msg.str.find("because:") != string::npos));
			  if (kick && SETTING(FILTER_MESSAGES)) {
				  addStatusMessage(Text::toT(msg.str));
			  } 
			  else if (!msg.from || !IgnoreManager::getInstance()->isIgnored(msg.from)) {
				  /* || (msg.fromId.isOp() && !client->isOp()) */ 
				  addChatMessage(msg.fromId, Text::toT(msg.str), WinUtil::m_ChatTextGeneral);
			  }
		  }
	  } 
	  else if (i->first == ADD_STATUS_LINE || i->first == ADD_SILENT_STATUS_LINE) {
		  addStatusMessage(Text::toT(static_cast<StringTask*>(i->second)->str));
	  } 
	  else if (i->first == SET_WINDOW_TITLE) {
		  SetWindowText(Text::toT(static_cast<StringTask*>(i->second)->str).c_str());
		  SetMDIFrameMenu();
	  } 
	  else if (i->first == STATS) {
		  m_infoPanel.updateData();
		  m_numberPanel.updateData();
	  } 
	  else if (i->first == GET_PASSWORD) {
		  if (!client->getPassword().empty()) {
			  client->password(client->getPassword());
			  addStatusMessage(TSTRING(STORED_PASSWORD_SENT));
		  } 
		  else {
			  if (!BOOLSETTING(PROMPT_PASSWORD)) {
				  if (!m_chatControl) {
					  activateChat(true);
				  }
				  m_chatControl->setMessageText(_T("/password "));
				  waitingForPW = true;
			  } 
			  else {
				  LineDlg linePwd;
				  linePwd.title = CTSTRING(ENTER_PASSWORD);
				  linePwd.description = CTSTRING(ENTER_PASSWORD);
				  linePwd.password = true;
				  //linePwd.disable = true;
				  if (linePwd.DoModal(m_hWnd) == IDOK) {
					  client->setPassword(Text::fromT(linePwd.line));
					  client->password(Text::fromT(linePwd.line));
					  waitingForPW = false;
				  } 
				  else {
					  client->disconnect(true);
				  }
			  }
		  }
	  } 
	  else if(i->first == PRIVATE_MESSAGE) {
		  MessageTask& pm = *static_cast<MessageTask*>(i->second);
		  if(!pm.from || !IgnoreManager::getInstance()->isIgnored(pm.from)) {
			  /* !SMT!-S || (pm.fromId.isOp() && !client->isOp()) */ 

			  tstring nick = Text::toT(pm.fromId.getNick());
			  bool myPM = (pm.replyTo == ClientManager::getInstance()->getMe());
			  const UserPtr& user = myPM ? pm.to : pm.replyTo;
			  bool isOpen = PrivateFrameFactory::isOpen(user);
			  if(pm.replyToId.isHub()) {
				  if(BOOLSETTING(IGNORE_HUB_PMS) && (Wildcard::patternMatch(Text::toT(user->getFirstNick()), opChat, ';', false) == false)) {
					  addStatusMessage(TSTRING(IGNORED_MESSAGE) + Text::toT(pm.str));
				  } else if(BOOLSETTING(POPUP_HUB_PMS) || isOpen) {
					  PrivateFrameFactory::gotMessage(pm.fromId, pm.to, pm.replyTo, Text::toT(pm.str), pm.annoying); // !SMT!-S
				  } else {
					  addStatusMessage(TSTRING(PRIVATE_MESSAGE_FROM) + nick + _T(": ") + Text::toT(pm.str));
				  }
			  } else if(pm.replyToId.isBot()) {
				  if(BOOLSETTING(IGNORE_BOT_PMS) && (Wildcard::patternMatch(Text::toT(user->getFirstNick()), opChat, ';', false) == false)) {
					  addStatusMessage(TSTRING(IGNORED_MESSAGE) + Text::toT(pm.str));
				  } else if(BOOLSETTING(POPUP_BOT_PMS) || isOpen) {
					  PrivateFrameFactory::gotMessage(pm.fromId, pm.to, pm.replyTo, Text::toT(pm.str), pm.annoying); // !SMT!-S
				  } else {
					  addStatusMessage(TSTRING(PRIVATE_MESSAGE_FROM) + nick + _T(": ") + Text::toT(pm.str));
				  }
			  } else {
				  // !SMT!-PSW
				  if (pm.annoying && BOOLSETTING(PROTECT_PRIVATE) && !isOpen && !FavoriteManager::getInstance()->hasFreePM(user) && !strstr(pm.str.c_str(), SETTING(PM_PASSWORD).c_str())) {
					  ClientManager::getInstance()->privateMessage(user, SETTING(PM_PASSWORD_HINT), false);
					  pm.annoying = false;
				  }
				  if(BOOLSETTING(POPUP_PMS) || isOpen) {
					  PrivateFrameFactory::gotMessage(pm.fromId, pm.to, pm.replyTo, Text::toT(pm.str), pm.annoying); // !SMT!-S
				  } else {
					  addStatusMessage(TSTRING(PRIVATE_MESSAGE_FROM) + nick + _T(": ") + Text::toT(pm.str));
				  }
				  if (pm.annoying) {
					  WinUtil::showPrivateMessageTrayIcon();
				  }										
			  }
		  }
	  } 
	  else if (i->first == CHEATING_USER) {
		  tstring msg = Text::toT(static_cast<StringTask*>(i->second)->str);
		  if(BOOLSETTING(POPUP_CHEATING_USER) && msg.length() < 256) {
			  WinUtil::ShowBalloonTip(msg.c_str(), CTSTRING(CHEATING_USER));
		  }
		  addStatusMessage(msg);
	  }

	  delete i->second;
  }
  for (vector<UserListControl*>::iterator i = m_userLists.begin(); i != m_userLists.end(); ++i) {
	  (*i)->endUpdate();
  }
  if (m_userListControl.isCountChanged()) {
	  m_numberPanel.updateData();
	  m_numberPanel.UpdateWindow();
  }
#ifdef _DEBUG
  __dcdebug(">>> onSpeaker tasks.size=%d elapsed=%u\n", t.size(), GetTickCount() - startTime);
#endif
  return 0;
}

bool HubFrame::isWrongNick() {
	static const tstring patterns[] = {
		_T("Никнейм занят другим пользователем"),
		_T("You are already in the hub"),
		_T("You have already logged into"),
		_T("Пользователь с таким ником уже зарегистрировался на хабе. Выберите другой ник."),
		_T("Ваш ник уже занят, пожалуйста измените на какой-нибудь другой!"),
		_T("Пользователь с таким ником уже подключён к хабу")
	};
	for (vector<tstring>::iterator i = lastMessages.begin(); i != lastMessages.end(); ++i) {
		tstring msg = *i;
		for (size_t j = 0; j < COUNTOF(patterns); ++j) {
			if (Util::findSubString(msg, patterns[j]) != wstring::npos) {
				return true;
			}
		}
	}
	return false;
}

void HubFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */) {
  RECT rect;
  GetClientRect(&rect);
  // position bars and offset their dimensions
  UpdateBarsPosition(rect, bResizeBars);

#ifdef HUB_FRAME_SPLITTER
  //SetSinglePaneMode(SPLIT_PANE_NONE);
  SetSplitterRect(&rect);
#else
  const int height = rect.bottom - rect.top;
  const int leftWidth = m_infoPanel.getBannerWidth();
  m_infoPanel.MoveWindow(rect.left, rect.top, leftWidth, height);
  const int userListLeft = rect.left + leftWidth + 2 * GetSystemMetrics(SM_CXEDGE);
  const int numberHeight = m_numberPanel.doLayout(rect.right - userListLeft);
  m_numberPanel.MoveWindow(userListLeft, rect.top + VPADDING , rect.right - userListLeft, numberHeight);
  //m_userListControl.MoveWindow(userListLeft, rect.top, rect.right - userListLeft, height - numberHeight);
#endif
}

LRESULT HubFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
  RecentHubEntry* r = FavoriteManager::getInstance()->getRecentHubEntry(Text::fromT(server));
  if (r) {
    TCHAR buf[256];
    this->GetWindowText(buf, 255);
    r->setName(Text::fromT(buf));
    r->setUsers(Util::toString(client->getUserCount()));
    r->setShared(Util::toString(client->getAvailable()));
    FavoriteManager::getInstance()->updateRecent(r);
  }

  SettingsManager::getInstance()->removeListener(this);
  TimerManager::getInstance()->removeListener(this);
  FavoriteManager::getInstance()->removeListener(this);
  client->removeListener(this);
  client->disconnect(true);
  if (m_chatFrame) {
    m_chatFrame->removeWindowListener(this);
    m_chatFrame->SendMessage(WM_CLOSE);
  }

  FavoriteManager::getInstance()->removeUserCommand(Text::fromT(server));

  clearUserList();
  clearTaskList();

  FavoriteHubEntry *fhe = m_userListControl.saveColumns(server);
  if (fhe != NULL) {
    WINDOWPLACEMENT wp;
    wp.length = sizeof(wp);
    GetWindowPlacement(&wp);

    CRect rc;
    GetWindowRect(rc);
    CRect rcmdiClient;
    ::GetWindowRect(WinUtil::mdiClient, &rcmdiClient);
    if (wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWNORMAL) {
      fhe->setWindowPosX(rc.left - (rcmdiClient.left + 2));
      fhe->setWindowPosY(rc.top - (rcmdiClient.top + 2));
      fhe->setWindowSizeX(rc.Width());
      fhe->setWindowSizeY(rc.Height());
    }
    if (wp.showCmd == SW_SHOWNORMAL || wp.showCmd == SW_SHOW || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_MAXIMIZE) {
      fhe->setWindowType((int)wp.showCmd);
    }
#ifdef HUB_FRAME_SPLITTER
    fhe->setChatUserSplit(m_nProportionalPos);
#endif

    FavoriteManager::getInstance()->save();
  }
  bHandled = FALSE;
  return 0;
}

void HubFrame::clearUserList() {
  for (vector<UserListControl*>::iterator i = m_userLists.begin(); i != m_userLists.end(); ++i) {
    (*i)->clear();
  }
  for (UserInfo::UserMapIter i = userMap.begin(); i != userMap.end(); ++i) {
    delete i->second;
  }
  userMap.clear();
}

void HubFrame::clearTaskList() {
  tasks.clear();
}

void HubFrame::onUserDblClick(UserInfo* ui, WPARAM wParam) {
  if (wParam & MK_CONTROL) { // MK_CONTROL = 0x0008
    PrivateFrameFactory::openWindow(ui->getUser());
  } 
  else if (wParam & MK_SHIFT) {
    try {
      QueueManager::getInstance()->addList(ui->getUser(), QueueItem::FLAG_CLIENT_VIEW);
    } 
    catch(const Exception& e) {
      addStatusMessage(Text::toT(e.getError()));
    }
  } 
  else {
    switch(SETTING(CHAT_DBLCLICK)) 
    {
    case 0:
		m_userLists[m_userLists.size()-1]->locateInList(ui);
      break;
    case 1:
      if (m_chatControl) m_chatControl->addUserToMessageText(ui->getText(COLUMN_NICK));      
      break;
    case 2:
      if (ui->getUser() != ClientManager::getInstance()->getMe()) {
        ui->pm();
      }
      break;
    case 3:
      ui->getList();
      break;
    case 4:
      ui->matchQueue();
      break;
    case 5:
      ui->grant();
      break;
    case 6:
      ui->addFav();
      break;
    }
  }
}

void HubFrame::addChatMessage(const Identity& i, const tstring& aLine, CHARFORMAT2& cf) {
  string extra;
  if(/* client->isOp() && !SMT!-S */ (BOOLSETTING(IP_IN_CHAT) || BOOLSETTING(COUNTRY_IN_CHAT))) {
    UserInfo* ui = findUser(Text::toT(WinUtil::findNickInTString(aLine)));
    if (ui) extra = WinUtil::getIpCountryForChat(ui->getIdentity().getIp(), m_chatControl && m_chatControl->isTimeStamps());
  }
  if(BOOLSETTING(LOG_MAIN_CHAT)) {
    StringMap params;
    params["message"] = Text::fromT(aLine);
    params["extra"] = extra;
    client->getHubIdentity().getParams(params, "hub", false);
    params["hubURL"] = client->getHubUrl();
    client->getMyIdentity().getParams(params, "my", true);
    LOG(LogManager::CHAT, params);
  }
  if (m_chatControl) {
    m_chatControl->addLine(i, client->getCurrentNick(), extra, aLine, cf, true);
    dcassert(m_chatFrame);
    //TODO if (BOOLSETTING(BOLD_HUB)) {
    m_chatFrame->setDirty();
  }
}

void HubFrame::addSystemMessage(const tstring& aLine) {
  if (m_chatControl) {
    m_chatControl->addSystemMessage(aLine);
    dcassert(m_chatFrame);
    //TODO if (BOOLSETTING(BOLD_HUB)) {
    m_chatFrame->setDirty();
  }
}

void HubFrame::addStatusMessage(const tstring& aLine) {
  addStatusMessage(aLine, Util::emptyStringT);
}

void HubFrame::addStatusMessage(const tstring& aLine, const tstring& author) {
	lastMessages.push_back(aLine);
	if (lastMessages.size() > 2) {
		lastMessages.erase(lastMessages.begin());
	}
  HubMessageControl::getInstance()->addMessage(hubName, author, aLine);
  if (BOOLSETTING(LOG_STATUS_MESSAGES)) {
    StringMap params;
    client->getHubIdentity().getParams(params, "hub", false);
    params["hubURL"] = client->getHubUrl();
    client->getMyIdentity().getParams(params, "my", true);
    params["message"] = Text::fromT(aLine);
    LOG(LogManager::STATUS, params);
  }
}

void HubFrame::addMessage(bool inChat, const tstring& aLine) {
  if (inChat) {
    addSystemMessage(aLine);
  }
  else {
    addStatusMessage(aLine);
  }
}

LRESULT HubFrame::onTabContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 
	tabMenuShown = true;
	CMenu hSysMenu;
	tabMenu.InsertSeparatorFirst(Text::toT((!client->getHubName().empty()) ? (client->getHubName().size() > 50 ? client->getHubName().substr(0, 50) : client->getHubName()) : client->getHubUrl()));	
	copyHubMenu.InsertSeparatorFirst(TSTRING(COPY));
	
	if(!client->isConnected())
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_GRAYED);
	else
		tabMenu.EnableMenuItem((UINT)(HMENU)copyHubMenu, MF_ENABLED);

	prepareMenu(tabMenu, ::UserCommand::CONTEXT_HUB, client->getHubUrl());
	hSysMenu.Attach((wParam == NULL) ? (HMENU)tabMenu : (HMENU)wParam);
	if (wParam != NULL) {
		hSysMenu.InsertMenu(hSysMenu.GetMenuItemCount() - 1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)(HMENU)tabMenu, /*CTSTRING(USER_COMMANDS)*/ _T("User Commands"));
		hSysMenu.InsertMenu(hSysMenu.GetMenuItemCount() - 1, MF_BYPOSITION | MF_SEPARATOR);
	}
	hSysMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
	if (wParam != NULL) {
		hSysMenu.RemoveMenu(hSysMenu.GetMenuItemCount() - 2, MF_BYPOSITION);
		hSysMenu.RemoveMenu(hSysMenu.GetMenuItemCount() - 2, MF_BYPOSITION);
	}
	cleanMenu(tabMenu);	
	tabMenu.RemoveFirstItem();
	copyHubMenu.RemoveFirstItem();
	hSysMenu.Detach();
	return TRUE;
}

LRESULT HubFrame::onCtlColor(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	HDC hDC = (HDC)wParam;
	::SetBkColor(hDC, WinUtil::bgColor);
	::SetTextColor(hDC, WinUtil::textColor);
	return (LRESULT)WinUtil::bgBrush;
}
	
bool HubFrame::onUserContextMenu(UserInfo* ui, CPoint pt) {
  tabMenuShown = false;
  UserPtr aSelectedUser = ui ? ui->getUser() : UserPtr(NULL);
  m_userListControl.setSelectedUserPtr(aSelectedUser);
  OMenu Mnu;
  if (PreparePopupMenu(true, aSelectedUser, &Mnu )) {
    if (!aSelectedUser) {
      Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
    } 
    else {
      WinUtil::clearSummaryMenu(); // !SMT!-UI
      UserInfoBase(aSelectedUser).addSummary(); // !SMT!-UI
      Mnu.AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::userSummaryMenu, CTSTRING(USER_SUMMARY)); // !SMT!-UI
      prepareMenu(Mnu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
      if(!(Mnu.GetMenuState(Mnu.GetMenuItemCount()-1, MF_BYPOSITION) & MF_SEPARATOR)) {
        Mnu.AppendMenu(MF_SEPARATOR);
      }
      Mnu.AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
      Mnu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);
      WinUtil::UnlinkStaticMenus(Mnu); // !SMT!-UI
      cleanMenu(Mnu);
    }
    if (Mnu != NULL) Mnu.DestroyMenu();
    return true;
  }
  return false;
}

void HubFrame::runUserCommand(::UserCommand& uc) {
  if(!WinUtil::getUCParams(m_hWnd, uc, ucLineParams))
    return;

  StringMap ucParams = ucLineParams;

  client->getMyIdentity().getParams(ucParams, "my", true);
  client->getHubIdentity().getParams(ucParams, "hub", false);

  if(tabMenuShown) {
    client->escapeParams(ucParams);
    client->sendUserCmd(Util::formatParams(uc.getCommand(), ucParams, false));
  } else {
    UserInfo* u = getSelectedUser();
    if (u && u->getUser()->isOnline()) {
      StringMap tmp = ucParams;
      u->getIdentity().getParams(tmp, "user", true);
      client->escapeParams(tmp);
      client->sendUserCmd(Util::formatParams(uc.getCommand(), tmp, false));
    } 
    else {
      vector<const UserInfo*> selectedUsers;
      m_userListControl.getSelectedUsers(selectedUsers);
      for (vector<const UserInfo*>::const_iterator i = selectedUsers.begin(); i != selectedUsers.end(); ++i) {
        const UserInfo* u = *i;
        if(u->getUser()->isOnline()) {
          StringMap tmp = ucParams;
          u->getIdentity().getParams(tmp, "user", true);
          client->escapeParams(tmp);
          client->sendUserCmd(Util::formatParams(uc.getCommand(), tmp, false));
        }
      }
    }
  }
}

LRESULT HubFrame::onFileReconnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	FavoriteHubEntry *fhe = FavoriteManager::getInstance()->getFavoriteHubEntry(Text::fromT(server));
	showJoins = (fhe ? (fhe->getShowJoins() || BOOLSETTING(SHOW_JOINS)) : BOOLSETTING(SHOW_JOINS));
	favShowJoins = BOOLSETTING(FAV_SHOW_JOINS);
	opChat = (fhe ? Text::toT(fhe->getOpChat()) : _T(""));
	exclChecks = (fhe ? fhe->getExclChecks() : false);

	client->reconnect();
	return 0;
}

LRESULT HubFrame::onFollow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(!redirect.empty()) {
		if(ClientManager::getInstance()->isConnected(Text::fromT(redirect))) {
			addStatusMessage(TSTRING(REDIRECT_ALREADY_CONNECTED));
			return 0;
		}
		
		dcassert(frames.find(server) != frames.end());
		dcassert(frames[server] == this);
		frames.erase(server);
		server = redirect;
		frames[server] = this;

		// the client is dead, long live the client!
		client->removeListener(this);
		ClientManager::getInstance()->putClient(client);
		clearUserList();
		clearTaskList();
		client = ClientManager::getInstance()->getClient(Text::fromT(server));

		RecentHubEntry r;
		r.setName("*");
		r.setDescription("***");
		r.setUsers("*");
		r.setShared("*");
		r.setServer(Text::fromT(redirect));
		FavoriteManager::getInstance()->addRecent(r);

		client->addListener(this);
		client->connect();
	}
	return 0;
}

void HubFrame::resortUsers() {
	for(FrameIter i = frames.begin(); i != frames.end(); ++i)
		i->second->resortForFavsFirst(true);
}

void HubFrame::closeDisconnected() {
	for(FrameIter i=frames.begin(); i!= frames.end(); ++i) {
		if (!(i->second->client->isConnected())) {
			i->second->PostMessage(WM_CLOSE);
		}
	}
}

void HubFrame::reconnectDisconnected() {
	for(FrameIter i=frames.begin(); i!= frames.end(); ++i) {
		if (!(i->second->client->isConnected())) {
			i->second->client->reconnect();
		}
	}
}

void HubFrame::closeAll(size_t thershold) {
	for(FrameIter i=frames.begin(); i!= frames.end(); ++i){
		if ( thershold == 0 || i->second->client->getUserCount() <= thershold  ) {
			i->second->PostMessage(WM_CLOSE);
		}
	}
}

void HubFrame::on(FavoriteManagerListener::UserAdded, const FavoriteUser& /*aUser*/) throw() {
	resortForFavsFirst();
}
void HubFrame::on(FavoriteManagerListener::UserRemoved, const FavoriteUser& /*aUser*/) throw() {
	resortForFavsFirst();
}

void HubFrame::resortForFavsFirst(bool justDoIt /* = false */) {
	if(justDoIt || BOOLSETTING(SORT_FAVUSERS_FIRST)) {
		resort = true;
		PostMessage(WM_SPEAKER);
	}
}

void HubFrame::on(Second, uint32_t /*aTick*/) throw() {
	if(updateUsers) {
		updateStatusBar();
		updateUsers = false;
		PostMessage(WM_SPEAKER);
	}
}

void HubFrame::on(Connecting, Client*) throw() { 
  //[-]PPA	if(BOOLSETTING(SEARCH_PASSIVE) && ClientManager::getInstance()->isActive(client->getHubUrl())) {
  //[-]PPA		add Line(TSTRING(ANTI_PASSIVE_SEARCH), WinUtil::m_ChatTextSystem);
  //[-]PPA	}
  speak(ADD_STATUS_LINE, STRING(CONNECTING_TO) + client->getHubUrl() + "...");
  speak(SET_WINDOW_TITLE, client->getHubUrl());
}

void HubFrame::on(Connected, Client* c) throw() {
  speak(CONNECTED);
  ChatBot::getInstance()->onHubAction(BotInit::RECV_CONNECT, c->getHubUrl());
}

void HubFrame::on(UserUpdated, Client*, const UserPtr& user) throw() { // !SMT!-fix
  speak(UPDATE_USER_JOIN, user);
  ChatBot::getInstance()->onUserAction(BotInit::RECV_UPDATE, user);
}

void HubFrame::on(UsersUpdated, Client*, const User::PtrList& aList) throw() {
  for (User::PtrList::const_iterator i = aList.begin(); i != aList.end(); ++i) {
    tasks.add(UPDATE_USER, new UserTask(*i, client)); // !SMT!-fix
    ChatBot::getInstance()->onUserAction(BotInit::RECV_UPDATE, *i);
  }
  updateUsers = true;
}

void HubFrame::on(ClientListener::UserRemoved, Client*, const UserPtr& user) throw() {
  speak(REMOVE_USER, user);
  ChatBot::getInstance()->onUserAction(BotInit::RECV_PART, user); // !SMT!-fix
}

void HubFrame::on(Redirect, Client*, const string& line) throw() { 
  if (ClientManager::getInstance()->isConnected(line)) {
    speak(ADD_STATUS_LINE, STRING(REDIRECT_ALREADY_CONNECTED));
    return;
  }
  redirect = Text::toT(line);
  if (BOOLSETTING(AUTO_FOLLOW)) {
    PostMessage(WM_COMMAND, IDC_FOLLOW, 0);
  }
  else {
    speak(ADD_STATUS_LINE, STRING(PRESS_FOLLOW) + line);
  }
}

void HubFrame::on(Failed, Client* c, const string& line) throw() {
  speak(ADD_STATUS_LINE, line); 
  speak(DISCONNECTED); 
  speak(STATS);
  ChatBot::getInstance()->onHubAction(BotInit::RECV_DISCONNECT, c->getHubUrl());
}

void HubFrame::on(GetPassword, Client*) throw() { 
  speak(GET_PASSWORD);
}

string HubFrame::getHubName() const throw() {
  string hubName = client->getHubName();
  if (!client->getHubDescription().empty()) {
    hubName += " - " + client->getHubDescription();
  }
  hubName += " (" + client->getHubUrl() + ")";
#ifdef _DEBUG
  string version = client->getHubIdentity().get("VE");
  if (!version.empty()) {
    hubName += " - " + version;
  }
#endif
  return hubName;
}
void HubFrame::on(HubUpdated, Client*) throw() { 
  hubName = Text::toT(client->getHubName());
	speak(SET_WINDOW_TITLE, getHubName());
}
void HubFrame::on(Message, Client* c, const UserPtr& from, const string& msg) throw() { // !SMT!-fix
  if (from != ClientManager::getInstance()->getMe()) {
    ChatBot::getInstance()->onMessage(c->getHubUrl(), from, Util::formatMessage(msg));
  }
  speak(ADD_CHAT_LINE, from, NULL, NULL, Util::formatMessage(msg));
}

void HubFrame::on(PrivateMessage, Client*, const UserPtr& from, const UserPtr to, const UserPtr replyTo, const string& line, bool annoying) throw() { // !SMT!-S
        speak(PRIVATE_MESSAGE, from, to, replyTo, Util::formatMessage(line), annoying); // !SMT!-S
}

void HubFrame::on(NickTaken, Client*) throw() { 
	speak(ADD_STATUS_LINE, STRING(NICK_TAKEN));
}

void HubFrame::on(SearchFlood, Client*, const string& line) throw() {
	speak(ADD_STATUS_LINE, STRING(SEARCH_SPAM_FROM) + line);
}

void HubFrame::on(CheatMessage, Client*, const string& line) throw() {
	speak(CHEATING_USER, line);
}

void HubFrame::on(HubTopic, Client*, const string& line) throw() {
	speak(ADD_STATUS_LINE, STRING(HUB_TOPIC) + "\t" + line);
}

void HubFrame::handleTab(bool reverse) {
	//HWND focus = GetFocus();

	if(reverse) {
		/*if(focus == ctrlFilterSel.m_hWnd) {
			ctrlFilter.SetFocus();
		} else if(focus == ctrlFilter.m_hWnd) {
			ctrlMessage.SetFocus();
		} else*/
          //if(focus == ctrlMessage.m_hWnd) {
	//		m_userListControl.setFocus();
		//} else if(focus == ctrlUsers.m_hWnd) {
		//	ctrlClient.SetFocus();
		//} else if(focus == ctrlClient.m_hWnd) {
			//ctrlFilterSel.SetFocus();
		//}
	} else {
		//if(focus == ctrlClient.m_hWnd) {
		//	m_userListControl.setFocus();
		//} else if(focus == ctrlUsers.m_hWnd) {
		//	ctrlMessage.SetFocus();
		//} else if(focus == ctrlMessage.m_hWnd) {
			//ctrlFilter.SetFocus();
		//} else if(focus == ctrlFilter.m_hWnd) {
		//	ctrlFilterSel.SetFocus();
		//} else if(focus == ctrlFilterSel.m_hWnd) {
		//	ctrlClient.SetFocus();
//		}
	}
}

void HubFrame::buildChatMenu(OMenu *pMenu) {
  if (!ChatCtrl::sSelectedIP.empty()) {
    pMenu->InsertSeparator(0, TRUE, ChatCtrl::sSelectedIP);
    pMenu->AppendMenu(MF_STRING, IDC_WHOIS_IP, (CTSTRING(WHO_IS) + ChatCtrl::sSelectedIP).c_str() );
    if (client->isOp()) {
      pMenu->AppendMenu(MF_SEPARATOR);
      pMenu->AppendMenu(MF_STRING, IDC_BAN_IP, (_T("!banip ") + ChatCtrl::sSelectedIP).c_str());
      pMenu->SetMenuDefaultItem(IDC_BAN_IP);
      pMenu->AppendMenu(MF_STRING, IDC_UNBAN_IP, (_T("!unban ") + ChatCtrl::sSelectedIP).c_str());
      pMenu->AppendMenu(MF_SEPARATOR);
    }
  } 
  else {
    pMenu->InsertSeparator(0, TRUE, _T("Text"));
  }
  pMenu->AppendMenu(MF_STRING, ID_EDIT_COPY, CTSTRING(COPY));
  pMenu->AppendMenu(MF_STRING, IDC_COPY_ACTUAL_LINE,  CTSTRING(COPY_LINE));
  if (m_chatControl && m_chatControl->isSelectedURL()) {
    pMenu->AppendMenu(MF_STRING, IDC_COPY_URL, CTSTRING(COPY_URL));
  }
  pMenu->AppendMenu(MF_SEPARATOR);
  pMenu->AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, CTSTRING(SELECT_ALL));
  pMenu->AppendMenu(MF_STRING, ID_EDIT_CLEAR_ALL, CTSTRING(CLEAR));
  pMenu->AppendMenu(MF_SEPARATOR);
  pMenu->AppendMenu(MF_STRING, IDC_AUTOSCROLL_CHAT, CTSTRING(ASCROLL_CHAT));
  if (m_chatControl && m_chatControl->isAutoScroll()) {
    pMenu->CheckMenuItem(IDC_AUTOSCROLL_CHAT, MF_BYCOMMAND | MF_CHECKED);
  }
}

void HubFrame::buildUserMenu(OMenu *pMenu, const UserPtr& aUser, bool bIsChat) {
  UserInfo *ui = findUser(aUser); // !SMT!-S
  bool bIsMe = (aUser == ClientManager::getInstance()->getMe());
  // Jediny nick
  pMenu->InsertSeparator(0, TRUE, Text::toT(aUser->getFirstNick()));
  // some commands starts in UserInfoBaseHandler, that requires user visible
  // in ListView. for now, just disable menu item to workaronud problem
  if (bIsChat) {
    if (!BOOLSETTING(LOG_PRIVATE_CHAT)) {
      pMenu->AppendMenu(MF_STRING | MF_DISABLED, (UINT_PTR)0,  CTSTRING(OPEN_USER_LOG));
    }
    else {
      pMenu->AppendMenu(MF_STRING, IDC_OPEN_USER_LOG,  CTSTRING(OPEN_USER_LOG));
    }
    pMenu->AppendMenu(MF_SEPARATOR);
  }
  if (!bIsChat && !bIsMe && BOOLSETTING(LOG_PRIVATE_CHAT)) {
    pMenu->AppendMenu(MF_STRING, IDC_OPEN_USER_LOG, CTSTRING(OPEN_USER_LOG));
    pMenu->AppendMenu(MF_SEPARATOR);
  }
  if (!bIsMe) {
    pMenu->AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
    pMenu->AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE));
  }
  if (bIsChat) {
    pMenu->AppendMenu(MF_STRING, IDC_SELECT_USER, CTSTRING(SELECT_USER_LIST));
  }
  if (!bIsMe) {
    pMenu->AppendMenu(MF_SEPARATOR);
  }
  pMenu->AppendMenu(MF_POPUP, (UINT) copyMenu.build(), CTSTRING(COPY));
  if (!bIsMe) {
    pMenu->AppendMenu(MF_POPUP, (UINT)(HMENU)WinUtil::grantMenu, CTSTRING(GRANT_SLOTS_MENU)); // !SMT!-UI
    if (ui) { // !SMT!-S
      pMenu->AppendMenu(MF_SEPARATOR);
	  if (!IgnoreManager::getInstance()->isIgnored(ui->getUser())) {
        pMenu->AppendMenu(MF_STRING, IDC_IGNORE, CTSTRING(IGNORE_USER));
      } 
      else {    
        pMenu->AppendMenu(MF_STRING, IDC_UNIGNORE, CTSTRING(UNIGNORE_USER));
      }
    }
    pMenu->AppendMenu(MF_SEPARATOR);
    pMenu->AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
    //pMenu->AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
    pMenu->AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
  }
}

static int chatDblClickCommands[] = {
  IDC_SELECT_USER,
  IDC_PUBLIC_MESSAGE,
  IDC_PRIVATEMESSAGE,
  IDC_GETLIST,
  IDC_MATCH_QUEUE,
  IDC_ADD_TO_FAVORITES
};

bool HubFrame::PreparePopupMenu(bool bIsChat, const UserPtr& aUser, OMenu *pMenu ) {
  if (pMenu->m_hMenu != NULL) {
    pMenu->DestroyMenu();
    pMenu->m_hMenu = NULL;
  }
  pMenu->CreatePopupMenu();
  if (bIsChat && !aUser) {
    buildChatMenu(pMenu);
  } 
  else {
    if (aUser) {
      buildUserMenu(pMenu, aUser, bIsChat);
    } 
    else {
      // Muze byt vice nicku
      const int iCount = m_userListControl.GetSelectedCount();
      if (!bIsChat) {
        // Pocet oznacenych
        pMenu->InsertSeparator(0, TRUE, Util::toStringW(iCount) + _T(" ") + TSTRING(HUB_USERS));
      }
      if (iCount <= 25) {
        pMenu->AppendMenu(MF_STRING, IDC_PUBLIC_MESSAGE, CTSTRING(SEND_PUBLIC_MESSAGE));
        pMenu->AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CTSTRING(SEND_PRIVATE_MESSAGE)); // !SMT!-UI
        pMenu->AppendMenu(MF_SEPARATOR);
      }
      pMenu->AppendMenu(MF_STRING, IDC_GETLIST, CTSTRING(GET_FILE_LIST));
      pMenu->AppendMenu(MF_STRING, IDC_MATCH_QUEUE, CTSTRING(MATCH_QUEUE));
      pMenu->AppendMenu(MF_STRING, IDC_ADD_TO_FAVORITES, CTSTRING(ADD_TO_FAVORITES));
    }
    if (bIsChat) {
      const int action = SETTING(CHAT_DBLCLICK);
      if (action >=0 && action < COUNTOF(chatDblClickCommands)) {
        pMenu->SetMenuDefaultItem(chatDblClickCommands[action]);
      }
    } 
    else {
      pMenu->SetMenuDefaultItem(m_userListControl.getDoubleClickAction());
    }
  }
  return true;
}

LRESULT HubFrame::onSelectUser(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  UserPtr& user = getSelectedUserPtr();
  if (user) {
    m_userLists[m_userLists.size()-1]->locateInList(Text::toT(user->getFirstNick()));
  }
  return 0;
}

LRESULT HubFrame::onPublicMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  if (!client->isConnected()) {
    return 0;
  }
  tstring sUsers = Util::emptyStringT;
  UserPtr& aSelectedUser = getSelectedUserPtr();
  if (aSelectedUser) {
    sUsers = Text::toT(aSelectedUser->getFirstNick()).c_str();
  } 
  else {
    vector<const UserInfo*> users;
    m_userListControl.getSelectedUsers(users);
    for (vector<const UserInfo*>::const_iterator i = users.begin(); i != users.end(); ++i) {
      const UserInfo* ui = *i;
      if (!sUsers.empty()) {
        sUsers += _T(", ");
      }
      sUsers += Text::toT(ui->getNick()).c_str();
    }
  }
  if (m_chatControl) {
    m_chatControl->addUserToMessageText(sUsers);
  }
  return 0;
}

LRESULT HubFrame::onBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ChatCtrl::sSelectedIP != _T("")) {
		tstring s = _T("!banip ") + ChatCtrl::sSelectedIP;

		client->hubMessage(Text::fromT(s));
	}
	return 0;
}

LRESULT HubFrame::onUnBanIP(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(ChatCtrl::sSelectedIP != _T("")) {
		tstring s = _T("!unban ") + ChatCtrl::sSelectedIP;

		client->hubMessage(Text::fromT(s));
	}
	return 0;
}

LRESULT HubFrame::onOpenUserLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	const UserInfo* ui;
	UserPtr& aSelectedUser = getSelectedUserPtr();
	if (aSelectedUser) {
		ui = findUser(aSelectedUser); // !SMT!-S
	}
	else {
		vector<const UserInfo*> users;
		m_userListControl.getSelectedUsers(users);
		if (users.size() == 0) {
			return 0;
		}
		ui = users[0];
	}
	if (ui == NULL) {
		return 0;
	}
	StringMap params;
	params["userNI"] = ui->getNick();
	params["hubNI"] = client->getHubName();
	params["myNI"] = client->getMyNick();
	params["userCID"] = ui->getUser()->getCID().toBase32();
	params["hubURL"] = client->getHubUrl();
	tstring file = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_PRIVATE_CHAT), params, false)));
	if(Util::fileExists(Text::fromT(file))) {
		if(BOOLSETTING(OPEN_LOGS_INTERNAL) == false) {
			ShellExecute(NULL, NULL, file.c_str(), NULL, NULL, SW_SHOWNORMAL);
		} else {
			TextFrame::openWindow(file);
		}
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_USER),CTSTRING(NO_LOG_FOR_USER), MB_OK );	  
	}
	return 0;
}

LRESULT HubFrame::onOpenHubLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	StringMap params;
	params["hubNI"] = client->getHubName();
	params["hubURL"] = client->getHubUrl();
	params["myNI"] = client->getMyNick(); 
	tstring filename = Text::toT(Util::validateFileName(SETTING(LOG_DIRECTORY) + Util::formatParams(SETTING(LOG_FILE_MAIN_CHAT), params, false)));
	if(Util::fileExists(Text::fromT(filename))){
		if(BOOLSETTING(OPEN_LOGS_INTERNAL) == false) {
		ShellExecute(NULL, NULL, filename.c_str(), NULL, NULL, SW_SHOWNORMAL);
	} else {
			TextFrame::openWindow(filename);
		}
	} else {
		MessageBox(CTSTRING(NO_LOG_FOR_HUB),CTSTRING(NO_LOG_FOR_HUB), MB_OK );	  
	}
	return 0;
}

void HubFrame::on(SettingsManagerListener::Save, SimpleXML& /*xml*/) throw() {
  m_userListControl.updateColors();
  RedrawWindow(NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

// !SMT!-UI
void HubFrame::addDupeUsersToSummaryMenu(const int64_t &share, const string& ip)
{
        for(FrameIter f=frames.begin(); f != frames.end(); ++f) {
                HubFrame *frame = f->second;
                for (UserInfo::UserMapIter i = frame->userMap.begin(); i != frame->userMap.end(); ++i) {
                        if (share && i->second->getIdentity().getBytesShared() == share) {
                                tstring info = Text::toT(frame->client->getHubName()) + _T(" - ") + i->second->getText(COLUMN_NICK);
                                UINT flags = (ip != Util::emptyString && ip == i->second->getIdentity().getIp())? MF_CHECKED : 0;
                                FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
                                FavoriteManager::FavoriteMap::const_iterator ii = favUsers.find(i->second->user->getCID());
                                if(ii != favUsers.end()) {
                                        const FavoriteUser& u = ii->second;
                                        string fav = Util::emptyString;
                                        if (u.isSet(FavoriteUser::FLAG_GRANTSLOT)) fav += " autoslot";
                                        if (u.isSet(FavoriteUser::FLAG_SUPERUSER)) fav += " su";
                                        if (u.isSet(FavoriteUser::FLAG_IGNOREPRIVATE)) fav += " ignore";
                                        if (u.getUploadLimit() != FavoriteUser::UL_NONE) fav += " " + FavoriteUser::GetLimitText(u.getUploadLimit());
                                        if (!u.getDescription().empty()) fav += " \"" + u.getDescription() + "\"";
                                        if (!fav.empty()) info += _T("\x09/") + Text::toT(fav);
                                }
                                WinUtil::userSummaryMenu.AppendMenu(MF_STRING | MF_DISABLED | flags, IDC_NONE, info.c_str());
                                WinUtil::userSummaryMenu.AppendMenu(MF_STRING | MF_DISABLED, IDC_NONE, Text::toT(i->second->getIdentity().getTag() + "\x09" + i->second->getIdentity().getIp()).c_str());
                        }
                }
        }
}

static TCHAR* mediaPlayerSpamCommands[] = {
  _T("/winamp"),
  _T("/wmp"),
  _T("/itunes"),
  _T("/mpc")
};

LRESULT HubFrame::onWinampSpam(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
  const int mediaPlayer = SETTING(MEDIA_PLAYER);
  if (mediaPlayer >= 0 && mediaPlayer < COUNTOF(mediaPlayerSpamCommands)) {
    tstring message, status;
    if (WinUtil::checkCommand(mediaPlayerSpamCommands[mediaPlayer], Util::emptyStringT, message, status)) {
      if (!message.empty()) {
        client->hubMessage(Text::fromT(message));
      }
      if (!status.empty()) {
        addStatusMessage(status);
      }
    }
  } 
  else {
    addStatusMessage(CTSTRING(NO_MEDIA_SPAM));
  }
  return 0;
}

HubFrame* HubFrame::getHub(Client* aClient) {
  for (FrameIter i = frames.begin() ; i != frames.end() ; ++i) {
    HubFrame* hubFrame = i->second;
    if (hubFrame->client == aClient) {
      return hubFrame;
    }
  }
  return NULL;
}

LRESULT HubFrame::onIgnore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
	if (client->isConnected()) {
		UserPtr& aSelectedUser = getSelectedUserPtr();
		if (aSelectedUser) {
			UserInfo* ui = findUser(aSelectedUser);
			if (ui) {
				IgnoreManager::getInstance()->storeIgnore(ui->getUser());
			}
		} 
		else {
			vector<const UserInfo*> users;
			m_userListControl.getSelectedUsers(users);
			for (vector<const UserInfo*>::const_iterator i = users.begin(); i != users.end(); ++i) {
				IgnoreManager::getInstance()->storeIgnore((*i)->getUser());
			}
		}
	}
	return 0;
}

LRESULT HubFrame::onUnignore(UINT /*uMsg*/, WPARAM /*wParam*/, HWND /*lParam*/, BOOL& /*bHandled*/) {
	if (client->isConnected()) {
		UserPtr& aSelectedUser = getSelectedUserPtr();
		if (aSelectedUser) {
			UserInfo* ui = findUser(aSelectedUser);
			if (ui) {
				IgnoreManager::getInstance()->removeIgnore(ui->getUser());
			}
		}
		else {
			vector<const UserInfo*> users;
			m_userListControl.getSelectedUsers(users);
			for (vector<const UserInfo*>::const_iterator i = users.begin(); i != users.end(); ++i) {
				IgnoreManager::getInstance()->removeIgnore((*i)->getUser());
			}
		}
	}
	return 0;
}

void HubFrame::onDoubleClickUser(UserListControl* control, UserInfo* ui) {
  switch (control->getDoubleClickAction()) 
  {
  case IDC_GETLIST:
    ui->getList();
    break;
  case IDC_PUBLIC_MESSAGE: 
    if (m_chatControl) {
      m_chatControl->addUserToMessageText(Text::toT(ui->getNick()));
    }
    break;
  case IDC_PRIVATEMESSAGE:
    if (ui->getUser() != ClientManager::getInstance()->getMe()) {
      ui->pm();
    }
    break;
  case IDC_MATCH_QUEUE:
    ui->matchQueue();
    break;
  case IDC_GRANTSLOT:
    ui->grant();
    break;
  case IDC_ADD_TO_FAVORITES:
    ui->addFav();
    break;
  }
}

bool HubFrame::buildUserListMenu(UserPtr& aSelectedUser, OMenu* menu) {
  PreparePopupMenu(false, aSelectedUser, menu);
  prepareMenu(*menu, ::UserCommand::CONTEXT_CHAT, client->getHubUrl());
  return true;
}

void HubFrame::cleanUserListMenu(OMenu* menu) {
  cleanMenu(*menu);
}

void HubFrame::userListMessage(const tstring& line) {
  addStatusMessage(line);
}

void HubFrame::autoCompleteFaliure() {
  m_userListControl.SetFocus();
}

void HubFrame::autoCompleteBegin() {
  m_userListControl.unselectAll();
}

tstring HubFrame::autoCompleteUserNick(const tstring& prefix) {
  return m_userListControl.autoCompleteUserNick(prefix);
}

#ifdef HUB_FRAME_SPLITTER
bool HubFrame::SetSplitterPos(int xyPos, bool bUpdate) {
  if (xyPos == -1) {
    xyPos = (m_rcSplitter.right - m_rcSplitter.left - m_cxySplitBar - m_cxyBarEdge) / 2;
  }
  if (m_infoPanel.isBannerActive()) {
    int bannerWidth = m_infoPanel.getBannerWidth();
    if (xyPos < bannerWidth) {
      xyPos = bannerWidth;
    }
  }
  return splitBase::SetSplitterPos(xyPos, bUpdate);
}
#endif

LRESULT HubFrame::onGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
  PChildMinMaxInfo(lParam)->m_minHeight = m_infoPanel.getMinHeight();
  return 0;
}

void HubFrame::on(UserIdentified, Client*) throw() {
	if (BOOLSETTING(OPEN_SUBSCRIPTIONS) && Util::findSubString(client->getHubUrl(), PeersUtils::PEERS_HUB) != string::npos) {
		speak(SUBSCRIPTIONS);
	}
}

WindowSortOrders HubFrame::getSortOrder() {
	return Util::findSubString(client->getHubUrl(), PeersUtils::PEERS_HUB) != string::npos ? SORT_ORDER_PEERS : SORT_ORDER_HUB_OTHER;
}

/**
* @file
* $Id: HubFrame.cpp,v 1.58.2.8 2008/12/30 02:03:29 alexey Exp $
*/
