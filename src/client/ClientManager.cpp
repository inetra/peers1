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

#include "stdinc.h"
#include "DCPlusPlus.h"

//[-]PPA [Doxygen 1.5.1] #include "ClientManager.h"

#include "ShareManager.h"
//[-]PPA [Doxygen 1.5.1] #include "SearchManager.h"
#include "CryptoManager.h"
#include "FavoriteManager.h"
#include "SimpleXML.h"
//[-]PPA [Doxygen 1.5.1] #include "UserCommand.h"
#include "ResourceManager.h"
//[-]PPA [Doxygen 1.5.1] #include "LogManager.h"

#include "AdcHub.h"
#include "NmdcHub.h"

//[-]PPA [Doxygen 1.5.1] #include <tchar.h> // !SMT!-IP

#include "QueueManager.h"
#include "FinishedManager.h"

Client* ClientManager::getClient(const string& aHubURL) {
	Client* c;
	if(Util::strnicmp("adc://", aHubURL.c_str(), 6) == 0) {
		c = new AdcHub(m_clientId, aHubURL, false);
	} else if(Util::strnicmp("adcs://", aHubURL.c_str(), 7) == 0) {
		c = new AdcHub(m_clientId, aHubURL, true);
	} else {
		c = new NmdcHub(m_clientId, aHubURL);
	}

	{
		Lock l(cs);
		clients.push_front(c);
	}

	c->addListener(this);

	return c;
}

void ClientManager::putClient(Client* aClient) {
	aClient->shutdown();

	fire(ClientManagerListener::ClientDisconnected(), aClient);
	aClient->removeListeners();

	{
		Lock l(cs);
		clients.remove(aClient);
	}
	delete aClient;
}

size_t ClientManager::getUserCount() const {
	Lock l(cs);
	return onlineUsers.size();
}

StringList ClientManager::getHubs(const CID& cid) const {
	Lock l(cs);
	StringList lst;
	OnlinePairC op = onlineUsers.equal_range(cid);
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubUrl());
	}
	return lst;
}

StringList ClientManager::getHubNames(const CID& cid) const {
	Lock l(cs);
	StringList lst;
	OnlinePairC op = onlineUsers.equal_range(cid);
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		lst.push_back(i->second->getClient().getHubName());
	}
	return lst;
}
/*
StringList ClientManager::getNicks(const CID& cid) const {
	Lock l(cs);
	StringSet nicks;
	OnlinePairC op = onlineUsers.equal_range(cid);
	for(OnlineIterC i = op.first; i != op.second; ++i) {
		nicks.insert(i->second->getIdentity().getNick());
	}
	if(nicks.empty()) {
		// Offline perhaps?
		UserMap::const_iterator i = users.find(cid);
		if(i != users.end() && !i->second->getFirstNick().empty()) {
			nicks.insert(i->second->getFirstNick());
		} else {
			nicks.insert('{' + cid.toBase32() + '}');
		}
	}
	return StringList(nicks.begin(), nicks.end());
}
*/
string ClientManager::getConnection(const CID& cid) const {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(cid);
	if(i != onlineUsers.end()) {
		return i->second->getIdentity().getConnection();
	}
	return STRING(OFFLINE);
}

int64_t ClientManager::getAvailable() const {
	Lock l(cs);
	int64_t bytes = 0;
	for(OnlineIterC i = onlineUsers.begin(); i != onlineUsers.end(); ++i) {
		bytes += i->second->getIdentity().getBytesShared();
	}

	return bytes;
}

bool ClientManager::isConnected(const string& aUrl) const {
	Lock l(cs);

	for(Client::List::const_iterator i = clients.begin(); i != clients.end(); ++i) {
		if((*i)->getHubUrl() == aUrl) {
			return true;
		}
	}
	return false;
}

// !SMT!-S
Client* ClientManager::findClient(const string& aUrl) const {
        Lock l(cs);
        for(Client::List::const_iterator i = clients.begin(); i != clients.end(); ++i) {
                if((*i)->getHubUrl() == aUrl) {
                        return (*i);
                }
        }
        return NULL;
}

string ClientManager::findHub(const string& ipPort) const {
	Lock l(cs);

	string ip;
	uint16_t port = 411;
	string::size_type i = ipPort.find(':');
	if(i == string::npos) {
		ip = ipPort;
	} else {
		ip = ipPort.substr(0, i);
		port = static_cast<uint16_t>(Util::toInt(ipPort.substr(i+1)));
	}

	string url;
	for(Client::List::const_iterator i = clients.begin(); i != clients.end(); ++i) {
		const Client* c = *i;
		if(c->getIp() == ip) {
			// If exact match is found, return it
			if(c->getPort() == port)
			return c->getHubUrl();

			// Port is not always correct, so use this as a best guess...
			url = c->getHubUrl();
		}
	}

	return url;
}

UserPtr ClientManager::findLegacyUser(const string& aNick) const throw() {
   Lock l(cs);
	if(aNick.size() > 0)
	{
	 for(OnlineMap::const_iterator i = onlineUsers.begin(); i != onlineUsers.end(); ++i) {
		const OnlineUser* ou = i->second;
		if(ou->getUser()->isSet(User::NMDC) && Util::stricmp(ou->getIdentity().getNick(), aNick) == 0)
			return ou->getUser();
	 }
	}
	return UserPtr();
}

UserPtr ClientManager::getUser(const string& aNick, const string& aHubUrl) throw() {
	CID cid = makeCid(aNick, aHubUrl);
	Lock l(cs);

	UserIter ui = users.find(cid);
	if(ui != users.end()) {
			ui->second->setFirstNick(aNick);	
		ui->second->setFlag(User::NMDC);
		return ui->second;
	}

	UserPtr p(new User(cid));
	p->setFirstNick(aNick);
	p->setFlag(User::NMDC);
	users.insert(make_pair(cid, p));

	return p;
}

UserPtr ClientManager::getUser(const CID& cid) throw() {
	Lock l(cs);
	UserMap::const_iterator ui = users.find(cid);
	if(ui != users.end()) {
		return ui->second;
	}

	UserPtr p(new User(cid));
	users.insert(make_pair(cid, p));
	return p;
}

UserPtr ClientManager::findUser(const CID& cid) const throw() {
	Lock l(cs);
	UserMap::const_iterator ui = users.find(cid);
	if(ui != users.end()) {
		return ui->second;
	}
	return 0;
}

bool ClientManager::isOp(const UserPtr& user, const string& aHubUrl) const {
	Lock l(cs);
	OnlinePairC p = onlineUsers.equal_range(user->getCID());
	for(OnlineIterC i = p.first; i != p.second; ++i) {
		if(i->second->getClient().getHubUrl() == aHubUrl) {
			return i->second->getIdentity().isOp();
		}
	}
	return false;
}

bool ClientManager::isStealth(const string& aHubUrl) const {
	Lock l(cs);
	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		const Client* c = *i;
		if(c->getHubUrl() == aHubUrl) {
			return c->getStealth();
		}
	}
	return false;
}

CID ClientManager::makeCid(const string& aNick, const string& aHubUrl) const throw() {
	string n = Text::toLower(aNick);
	TigerHash th;
	th.update(n.c_str(), n.length());
	th.update(Text::toLower(aHubUrl).c_str(), aHubUrl.length());
	// Construct hybrid CID from the bits of the tiger hash - should be
	// fairly random, and hopefully low-collision
	return CID(th.finalize());
}

void ClientManager::putOnline(OnlineUser* ou) throw() {
	{
		Lock l(cs);
		onlineUsers.insert(make_pair(ou->getUser()->getCID(), ou));
	}

	if(!ou->getUser()->isOnline()) {
		ou->getUser()->setFlag(User::ONLINE);
		ou->getIdentity().set("LI", Util::toString(GET_TICK()));
		ou->getIdentity().set("LT", Util::formatTime("%d-%m %H:%M", GET_TIME()));
		fire(ClientManagerListener::UserConnected(), ou->getUser());
	}
}

void ClientManager::putOffline(OnlineUser* ou) throw() {
	bool lastUser = false;
	{
		Lock l(cs);
		OnlinePair op = onlineUsers.equal_range(ou->getUser()->getCID());
		dcassert(op.first != op.second);
		for(OnlineIter i = op.first; i != op.second; ++i) {
			OnlineUser* ou2 = i->second;
			if(ou == ou2) {
				lastUser = (distance(op.first, op.second) == 1);
				onlineUsers.erase(i);
				break;
			}
		}
	}

	if(lastUser) {
		ou->getUser()->unsetFlag(User::ONLINE);
		fire(ClientManagerListener::UserDisconnected(), ou->getUser());
	}
}

void ClientManager::connect(const UserPtr& p) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(p->getCID());
	if(i != onlineUsers.end()) {
		OnlineUser* u = i->second;
		u->getClient().connect(*u);
	}
}

void ClientManager::privateMessage(const UserPtr& p, const string& msg, bool annoying) { // !SMT!-S
        Client* cli = NULL; // !SMT!-S
        {
	Lock l(cs);
                OnlineUser* ou = getOnlineUser(p);
                if (ou) cli = &ou->getClient();
	}
        if (cli)
                cli->privateMessage(p, msg, annoying);  // !SMT!-S
}

void ClientManager::send(AdcCommand& cmd, const CID& cid) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(cid);
	if(i != onlineUsers.end()) {
		OnlineUser& u = *i->second;
		if(cmd.getType() == AdcCommand::TYPE_UDP && !u.getIdentity().isUdpActive()) {
			cmd.setType(AdcCommand::TYPE_DIRECT);
			cmd.setTo(u.getIdentity().getSID());
			u.getClient().send(cmd);
		} else {
			try {
				Socket udp;
				udp.writeTo(u.getIdentity().getIp(), static_cast<uint16_t>(Util::toInt(u.getIdentity().getUdpPort())), cmd.toString(getMe()->getCID()));
			} catch(const SocketException&) {
				dcdebug("Socket exception sending ADC UDP command\n");
			}
		}
	}
}

void ClientManager::infoUpdated(bool antispam) {
	if(GET_TICK() > (quickTick + 10000) || antispam == false) {
		Lock l(cs);
		for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
			if((*i)->isConnected()) {
				(*i)->info();
			}
		}
		quickTick = GET_TICK();
	}
}

void ClientManager::on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize, 
									int aFileType, const string& aString, bool isPassive) throw() 
{
        ClientManagerListener::SearchReply re = ClientManagerListener::SEARCH_MISS; // !SMT!-S

        /* !SMT!-S
	// We don't wan't to answer passive searches if we're in passive mode...
	if(isPassive && !ClientManager::getInstance()->isActive(aClient->getHubUrl())) {
		return;
	}
        */

	SearchResult::List l;
	ShareManager::getInstance()->search(l, aString, aSearchType, aSize, aFileType, aClient, isPassive ? 5 : 10);
	if(l.size() > 0) {
                re = ClientManagerListener::SEARCH_HIT;
		if(isPassive) {
			string name = aSeeker.substr(4);
			// Good, we have a passive seeker, those are easier...
			string str;
			for(SearchResult::Iter i = l.begin(); i != l.end(); ++i) {
				SearchResult* sr = *i;
				str += sr->toSR(*aClient);
				str[str.length()-1] = 5;
				str += name;
				str += '|';

				sr->decRef();
			}
			
			if(str.size() > 0)
				aClient->send(str);
			
		} else {
			try {
				Socket udp;
				string ip, file;
				uint16_t port = 0;
				Util::decodeUrl(aSeeker, ip, port, file);
				ip = Socket::resolve(ip);
				
				if(port == 0) 
					port = 412;
				for(SearchResult::Iter i = l.begin(); i != l.end(); ++i) {
					SearchResult* sr = *i;
					udp.writeTo(ip, port, sr->toSR(*aClient));
					sr->decRef();
				}
			} catch(...) {
				for(SearchResult::Iter i = l.begin(); i != l.end(); ++i) {
					SearchResult* sr = *i;
					sr->decRef();
				}
				LOG_MESSAGE("Search caught error");
			}
		}
	} else if(!isPassive && (aFileType == SearchManager::TYPE_TTH) && (aString.compare(0, 4, "TTH:") == 0)) {
		string ip, file;
		uint16_t port = 0;
		Util::decodeUrl(aSeeker, ip, port, file);
		FileChunksInfo::Ptr chunksInfo = NULL;
		PartsInfo partialInfo;
		TTHValue aTTH(aString.substr(4));
		if(QueueManager::getInstance()->handlePartialSearch(aTTH, chunksInfo) && chunksInfo) {
                        re = ClientManagerListener::SEARCH_PARTIAL_HIT; // !SMT!-S
			// we have a search for something we are downloading, give a partial result
			if(port == 0) port = 412;

			PartialPeer partialPeer(aClient->getMyNick(), aClient->getIpPort(), aTTH.toBase32(), ip, port, false);

			// send the psr, and ask for a reply
			chunksInfo->sendPSR(partialPeer, true);
			chunksInfo->storePartialPeer(partialPeer);
		}else if(FinishedManager::getInstance()->handlePartialRequest(aTTH, partialInfo)){
                        re = ClientManagerListener::SEARCH_PARTIAL_HIT; // !SMT!-S
			// if not found, try to find in finished list
			SearchManager::getInstance()->sendPSR(ip, port, true, aClient->getMyNick(), aClient->getIpPort(), aTTH.toBase32(), partialInfo);
		}
	}
        Speaker<ClientManagerListener>::fire(ClientManagerListener::IncomingSearch(), aSeeker, aString, re); // !SMT!-S
}

void ClientManager::userCommand(const UserPtr& p, const ::UserCommand& uc, StringMap& params, bool compatibility) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(p->getCID());
	if(i == onlineUsers.end())
		return;

	OnlineUser& ou = *i->second;
	ou.getIdentity().getParams(params, "user", compatibility);
	ou.getClient().getHubIdentity().getParams(params, "hub", false);
	ou.getClient().getMyIdentity().getParams(params, "my", compatibility);
	ou.getClient().escapeParams(params);
	ou.getClient().sendUserCmd(Util::formatParams(uc.getCommand(), params, false));
}

void ClientManager::on(AdcSearch, Client*, const AdcCommand& adc, const CID& from) throw() {
	SearchManager::getInstance()->respond(adc, from);
}

const string& ClientManager::getHubUrl(const UserPtr& aUser) const {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(aUser->getCID());
	if(i != onlineUsers.end()) {
		return i->second->getClient().getHubUrl();
	}
	return Util::emptyString;
}

void ClientManager::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken) {
	Lock l(cs);

	for(Client::Iter i = clients.begin(); i != clients.end(); ++i) {
		if((*i)->isConnected()) {
			(*i)->search(aSizeMode, aSize, aFileType, aString, aToken);
		}
	}
}

void ClientManager::search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken) {
	Lock l(cs);
	for(StringIter it = who.begin(); it != who.end(); ++it) {
		string& client = *it;
		for(Client::Iter j = clients.begin(); j != clients.end(); ++j) {
			Client* c = *j;
			if(c->isConnected() && c->getHubUrl() == client) {
				c->search(aSizeMode, aSize, aFileType, aString, aToken);
			}
		}
	}
}

void ClientManager::on(Load, SimpleXML&) throw() {
	users.insert(make_pair(getMe()->getCID(), getMe()));
}

void ClientManager::on(TimerManagerListener::Minute, uint32_t /*aTick*/) throw() {
	{
		Lock l(cs);

		// Collect some garbage...
		UserIter i = users.begin();
		while(i != users.end()) {
			if(i->second->unique()) {
				users.erase(i++);
			} else {
				++i;
			}
		}

		for(Client::Iter j = clients.begin(); j != clients.end(); ++j) {
			(*j)->info();
		}
	}
        /* !SMT!-F  no point
	if((aTick / 60000) % 5 == 0)
		SetProcessWorkingSetSize(GetCurrentProcess(), 0xffffffff, 0xffffffff);
        */
}

UserPtr& ClientManager::getMe() {
	if(!me) {
		Lock l(cs);
		if(!me) {
			me = new User(getMyCID());
			me->setFirstNick(SETTING(NICK));
		}
	}
	return me;
}

const CID& ClientManager::getMyPID() {
	if(pid.isZero())
		pid = CID(SETTING(PRIVATE_ID));
	return pid;
}

CID ClientManager::getMyCID() {
	TigerHash tiger;
	tiger.update(getMyPID().data(), CID::SIZE);
	return CID(tiger.finalize());
}

void ClientManager::on(Failed, Client* client, const string&) throw() { 
	FavoriteManager::getInstance()->removeUserCommand(client->getHubUrl());
	fire(ClientManagerListener::ClientDisconnected(), client);
}

void ClientManager::on(UserCommand, Client* client, int aType, int ctx, const string& name, const string& command) throw() { 
	if(BOOLSETTING(HUB_USER_COMMANDS)) {
		if(aType == ::UserCommand::TYPE_CLEAR) {
 			FavoriteManager::getInstance()->removeHubUserCommands(ctx, client->getHubUrl());
 		} else {
			FavoriteManager::getInstance()->addUserCommand(aType, ctx, ::UserCommand::FLAG_NOSAVE, name, command, client->getHubUrl());
		}
	}
}


void ClientManager::setListLength(const UserPtr& p, const string& listLen) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(p->getCID());
	if(i != onlineUsers.end()) {
		i->second->getIdentity().set("LL", listLen);
	}
}

void ClientManager::fileListDisconnected(const UserPtr& p) {
	string report = Util::emptyString;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			OnlineUser& ou = *i->second;
	
			int fileListDisconnects = Util::toInt(ou.getIdentity().get("FD")) + 1;
			ou.getIdentity().set("FD", Util::toString(fileListDisconnects));

			if(SETTING(ACCEPTED_DISCONNECTS) == 0)
				return;

			if(fileListDisconnects == SETTING(ACCEPTED_DISCONNECTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClient(), "Disconnected file list " + Util::toString(fileListDisconnects) + " times", false);
				ou.getIdentity().sendRawCommand(ou.getClient(), SETTING(DISCONNECT_RAW));
			}
		}
	}
	if(c && !report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT)) {
		c->cheatMessage(report);
	}
}

void ClientManager::connectionTimeout(const UserPtr& p) {
	string report = Util::emptyString;
	bool remove = false;
	Client* c = NULL;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			OnlineUser& ou = *i->second;
	
			int connectionTimeouts = Util::toInt(ou.getIdentity().get("TO")) + 1;
			ou.getIdentity().set("TO", Util::toString(connectionTimeouts));
	
			if(SETTING(ACCEPTED_TIMEOUTS) == 0)
				return;
	
			if(connectionTimeouts == SETTING(ACCEPTED_TIMEOUTS)) {
				c = &ou.getClient();
				report = ou.getIdentity().setCheat(ou.getClient(), "Connection timeout " + Util::toString(connectionTimeouts) + " times", false);
				remove = true;
				ou.getIdentity().sendRawCommand(ou.getClient(), SETTING(TIMEOUT_RAW));
			}
		}
	}
	if(remove) {
		try {
			QueueManager::getInstance()->removeTestSUR(p);
		} catch(...) {
		}
	}
	if(c && !report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT)) {
		c->cheatMessage(report);
	}
}

void ClientManager::checkCheating(const UserPtr& p, DirectoryListing* dl) {
	string report = Util::emptyString;
	OnlineUser* ou = NULL;
	{
		Lock l(cs);

		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end())
			return;

		ou = i->second;

		int64_t statedSize = ou->getIdentity().getBytesShared();
		int64_t realSize = dl->getTotalSize();
	
		double multiplier = ((100+(double)SETTING(PERCENT_FAKE_SHARE_TOLERATED))/100); 
		int64_t sizeTolerated = (int64_t)(realSize*multiplier);
		string detectString = Util::emptyString;
		string inflationString = Util::emptyString;
		ou->getIdentity().set("RS", Util::toString(realSize));
		bool isFakeSharing = false;
	
		if(statedSize > sizeTolerated) {
			isFakeSharing = true;
		}

		if(isFakeSharing) {
			ou->getIdentity().setBF(true);
			detectString += STRING(CHECK_MISMATCHED_SHARE_SIZE);
			if(realSize == 0) {
				detectString += STRING(CHECK_0BYTE_SHARE);
			} else {
				double qwe = (double)((double)statedSize / (double)realSize);
				char buf[128];
				snprintf(buf, sizeof(buf), CSTRING(CHECK_INFLATED), Util::toString(qwe).c_str());
				inflationString = buf;
				detectString += inflationString;
			}
			detectString += STRING(CHECK_SHOW_REAL_SHARE);

			report = ou->getIdentity().setCheat(ou->getClient(), detectString, false);
			ou->getIdentity().sendRawCommand(ou->getClient(), SETTING(FAKESHARE_RAW));
		}
		ou->getIdentity().setFC(true);
		ou->getIdentity().set("FQ", Util::emptyString);
		ou->getIdentity().set("FT", Util::toString(GET_TIME()));
	}
	ou->getClient().updated(*ou);
	if(!report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT))
		ou->getClient().cheatMessage(report);
}

void ClientManager::setCheating(const UserPtr& p, const string& aTestSURString, const string& aCheatString, const int aRawCommand, bool aBadClient) {
	OnlineUser* ou = NULL;
	string report = Util::emptyString;
	{
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;
		
		ou = i->second;
		
		if(!aTestSURString.empty()) {
			ou->getIdentity().set("TS", aTestSURString);
			ou->getIdentity().setTC(true);
			ou->getIdentity().set("TQ", Util::emptyString);
			ou->getIdentity().set("TT", Util::toString(GET_TIME()));
			report = ou->getIdentity().updateClientType(*ou);
		}
		if(!aCheatString.empty()) {
			report = ou->getIdentity().setCheat(ou->getClient(), aCheatString, aBadClient);
		}
		if(aRawCommand != -1)
			ou->getIdentity().sendRawCommand(ou->getClient(), aRawCommand);
	}
	ou->getClient().updated(*ou);
	if(!report.empty() && BOOLSETTING(DISPLAY_CHEATS_IN_MAIN_CHAT))
		ou->getClient().cheatMessage(report);
}

void ClientManager::setFakeList(const UserPtr& p, const string& aCheatString) {
	Lock l(cs);
	OnlineIterC i = onlineUsers.find(p->getCID());
	if(i == onlineUsers.end()) return;

	i->second->getIdentity().setBF(true);
	i->second->getIdentity().setCheat(i->second->getClient(), aCheatString, false);
}

int ClientManager::getMode(const string& aHubUrl) {
	if(aHubUrl.empty()) return SETTING(INCOMING_CONNECTIONS);

	int mode = 0;
	FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(aHubUrl);
	if(hub) {
		switch(hub->getMode()) {
			case 1 :
				mode = SettingsManager::INCOMING_DIRECT;
				break;
			case 2 :
				mode = SettingsManager::INCOMING_FIREWALL_PASSIVE;
				break;
			default:
				mode = SETTING(INCOMING_CONNECTIONS);
		}
	} else {
		mode = SETTING(INCOMING_CONNECTIONS);
	}
	return mode;
}

// !SMT!-IP
void ClientManager::logUserIp(const string& IP, const UserPtr& user)
{
	if(m_GuardOpenFile) //[+]PPA	  
	   return;
   const string cid = user->getCID().toBase32();
   const string str = cid+IP;
   if (loggedUserIps.find(str) == loggedUserIps.end()) {
      loggedUserIps.insert(str);
      TCHAR logFileName[512];
      ::GetModuleFileName(0, logFileName, 512);
      _tcscpy(_tcsrchr(logFileName, '\\'), L"\\userip.log");
      if (FILE *ff = _wfopen(logFileName, L"rt+")) {
         fseek(ff, 0, SEEK_END);
         string hubip;
         StringList lst = getHubs(user->getCID());
         if (lst.size() > 0) hubip = *(lst.begin());
         string nick = user->getFirstNick();
         if (nick.find('\'') != string::npos)
                for (unsigned i = 0; i < nick.length(); i++)
                        if (nick[i] == '\'') nick[i] = '\"';
         SYSTEMTIME t; ::GetLocalTime(&t);
         fprintf(ff,
            "insert into IPLOG(STAMP,CID,HUBIP,NICK,USERIP) values "
            "('%02d.%02d.%04d %02d:%02d', '%s', '%s', '%s', '%s');\n",
            t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute,
            cid.c_str(), hubip.c_str(), nick.c_str(), IP.c_str());
         fclose(ff);
      }
	  else
       m_GuardOpenFile = true; //[+]PPA
   }
}

/**
 * @file
 * $Id: ClientManager.cpp,v 1.10 2008/12/20 04:25:03 alexey Exp $
 */
