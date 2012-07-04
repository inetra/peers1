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

#include "ConnectionManager.h"

#include "ResourceManager.h"
#include "DownloadManager.h"
#include "UploadManager.h"
#include "CryptoManager.h"
//[-]PPA [Doxygen 1.5.1] #include "ClientManager.h"
#include "QueueManager.h"
//[-]PPA [Doxygen 1.5.1] #include "LogManager.h"

//[-]PPA [Doxygen 1.5.1] #include "UserConnection.h"
#include "PGLoader.h"
#include "FavoriteManager.h"
bool ConnectionManager::m_DisableAutoBan = false; //[+]PPA

ConnectionManager::ConnectionManager() : floodCounter(0), server(0), 
#ifdef PPA_INCLUDE_SSL
  secureServer(0),
#endif
    shuttingDown(false) {
	TimerManager::getInstance()->addListener(this);

	features.push_back(UserConnection::FEATURE_MINISLOTS);
	features.push_back(UserConnection::FEATURE_XML_BZLIST);
	features.push_back(UserConnection::FEATURE_ADCGET);
	features.push_back(UserConnection::FEATURE_TTHL);
	features.push_back(UserConnection::FEATURE_TTHF);
        features.push_back(UserConnection::FEATURE_BANMSG); // !SMT!-B

	adcFeatures.push_back("AD" + UserConnection::FEATURE_ADC_BASE);
	adcFeatures.push_back("AD" + UserConnection::FEATURE_ADC_BZIP);
}

void ConnectionManager::listen() throw(SocketException){
	disconnect();
	uint16_t port = static_cast<uint16_t>(SETTING(TCP_PORT));

	server = new Server(false, port, SETTING(BIND_ADDRESS));

#ifdef PPA_INCLUDE_SSL
	if(!CryptoManager::getInstance()->TLSOk()) {
		dcdebug("Skipping secure port: %d\n", SETTING(USE_TLS));
                return;
	}

	port = static_cast<uint16_t>(SETTING(TLS_PORT));

	secureServer = new Server(true, port, SETTING(BIND_ADDRESS));
#else
			return;
#endif
}

/**
 * Request a connection for downloading.
 * DownloadManager::addConnection will be called as soon as the connection is ready
 * for downloading.
 * @param aUser The user to connect to.
 */
void ConnectionManager::getDownloadConnection(const UserPtr& aUser) {
	dcassert((bool)aUser);
	{
		Lock l(cs);
		ConnectionQueueItem::Iter i = find(downloads.begin(), downloads.end(), aUser);
		if(i == downloads.end()) {
			getCQI(aUser, true);
		} else {
			if(find(checkIdle.begin(), checkIdle.end(), aUser) == checkIdle.end())
				checkIdle.push_back(aUser);
		}
	}
}

ConnectionQueueItem* ConnectionManager::getCQI(const UserPtr& aUser, bool download) {
	ConnectionQueueItem* cqi = new ConnectionQueueItem(aUser, download);
	if(download) {
		dcassert(find(downloads.begin(), downloads.end(), aUser) == downloads.end());
		downloads.push_back(cqi);
	} else {
		dcassert(find(uploads.begin(), uploads.end(), aUser) == uploads.end());
		uploads.push_back(cqi);
	}

	fire(ConnectionManagerListener::Added(), cqi);
	return cqi;
}

void ConnectionManager::putCQI(ConnectionQueueItem* cqi) {
	fire(ConnectionManagerListener::Removed(), cqi);
	if(cqi->getDownload()) {
		downloads.erase_and_check(cqi);
	} else {
		UploadManager::getInstance()->removeDelayUpload(cqi->getUser());
		uploads.erase_and_check(cqi);
	}
	delete cqi;
}

UserConnection* ConnectionManager::getConnection(bool aNmdc, bool secure) throw() {
	UserConnection* uc = new UserConnection(secure);
	uc->addListener(this);
	{
		Lock l(cs);
		userConnections.push_back(uc);
	}
	if(aNmdc)
		uc->setFlag(UserConnection::FLAG_NMDC);
	return uc;
}

void ConnectionManager::putConnection(UserConnection* aConn) {
	aConn->removeListener(this);
	aConn->disconnect(true);
	Lock l(cs);
	userConnections.erase_and_check(aConn);
}

void ConnectionManager::on(TimerManagerListener::Second, uint32_t aTick) throw() {
	UserList passiveUsers;
	ConnectionQueueItem::List removed;
	UserList idlers;

	{
		Lock l(cs);

		uint16_t attempts = 0;

		idlers = checkIdle;
		checkIdle.clear();

		for(ConnectionQueueItem::Iter i = downloads.begin(); i != downloads.end(); ++i) 
		{
			ConnectionQueueItem* cqi = *i;
            if(!cqi) continue; //[+]PPA
			if(cqi->getState() != ConnectionQueueItem::ACTIVE) {
				if(!cqi->getUser()->isOnline()) {
					// Not online anymore...remove it from the pending...
					removed.push_back(cqi);
					continue;
				} 
				
				if(	cqi->getUser()->isSet(User::PASSIVE) &&
					!ClientManager::getInstance()->isActive(ClientManager::getInstance()->getHubUrl(cqi->getUser()))) {
					passiveUsers.push_back(cqi->getUser());
					removed.push_back(cqi);
					continue;
				}

                                if( cqi->getLastAttempt() == 0 || //[+] DC++ r722
									((cqi->getLastAttempt() + 60*1000) < aTick) && ((SETTING(DOWNCONN_PER_SEC)== 0) || (attempts < SETTING(DOWNCONN_PER_SEC))) )
                                {
					cqi->setLastAttempt(aTick);

					QueueItem::Priority prio = QueueManager::getInstance()->hasDownload(cqi->getUser());

					if(prio == QueueItem::PAUSED) {
						removed.push_back(cqi);
						continue;
					}

					bool startDown = DownloadManager::getInstance()->startDownload(prio);

					if(cqi->getState() == ConnectionQueueItem::WAITING) {
						if(startDown) {
							cqi->setState(ConnectionQueueItem::CONNECTING);
							ClientManager::getInstance()->connect(cqi->getUser());
							fire(ConnectionManagerListener::StatusChanged(), cqi);
							attempts++;
						} else {
							cqi->setState(ConnectionQueueItem::NO_DOWNLOAD_SLOTS);
							fire(ConnectionManagerListener::Failed(), cqi, STRING(ALL_DOWNLOAD_SLOTS_TAKEN));
						}
					} else if(cqi->getState() == ConnectionQueueItem::NO_DOWNLOAD_SLOTS && startDown) {
						cqi->setState(ConnectionQueueItem::WAITING);
					}
				} else if(((cqi->getLastAttempt() + 50*1000) < aTick) && (cqi->getState() == ConnectionQueueItem::CONNECTING)) {
					ClientManager::getInstance()->connectionTimeout(cqi->getUser());

					fire(ConnectionManagerListener::Failed(), cqi, STRING(CONNECTION_TIMEOUT));
					cqi->setState(ConnectionQueueItem::WAITING);
				}
			}
		}

		for(ConnectionQueueItem::Iter m = removed.begin(); m != removed.end(); ++m) {
			putCQI(*m);
		}

	}

	for(UserList::const_iterator i = idlers.begin(); i != idlers.end(); ++i) {
		DownloadManager::getInstance()->checkIdle(*i);
	}

	for(UserList::iterator ui = passiveUsers.begin(); ui != passiveUsers.end(); ++ui) {
		QueueManager::getInstance()->removeSource(*ui, QueueItem::Source::FLAG_PASSIVE);
	}
}

void ConnectionManager::on(TimerManagerListener::Minute, uint32_t aTick) throw() {	
	Lock l(cs);

	for(UserConnection::Iter j = userConnections.begin(); j != userConnections.end(); ++j) {
		if(((*j)->getLastActivity() + 180*1000) < aTick) 
		{
			(*j)->disconnect(true);
		}
	}
}

static const uint32_t FLOOD_TRIGGER = 20000;
static const uint32_t FLOOD_ADD = 2000;

ConnectionManager::Server::Server(bool 
#ifdef PPA_INCLUDE_SSL
								  secure_
#endif
								  , uint16_t aPort, const string& ip /* = "0.0.0.0" */) : 
port(0), 
#ifdef PPA_INCLUDE_SSL
secure(secure_), 
#endif
die(false) {
	sock.create();
	port = sock.bind(aPort, ip);
	sock.listen();
	start();
}

static const uint32_t POLL_TIMEOUT = 250;

int ConnectionManager::Server::run() throw() {
	try {
		while(!die) {
			if(sock.wait(POLL_TIMEOUT, Socket::WAIT_READ) == Socket::WAIT_READ) {
				ConnectionManager::getInstance()->accept(sock,
#ifdef PPA_INCLUDE_SSL
					, secure
#else
   					 false
#endif
					);
			}
		}
	} catch(const Exception& e) {
		LogManager::getInstance()->message(STRING(LISTENER_FAILED) + e.getError());
	}
	return 0;
}

/**
 * Someone's connecting, accept the connection and wait for identification...
 * It's always the other fellow that starts sending if he made the connection.
 */
void ConnectionManager::accept(const Socket& sock, bool secure) throw() {
	uint64_t now = GET_TICK();

	if(now > floodCounter) {
		floodCounter = now + FLOOD_ADD;
	} else {
		if(false && now + FLOOD_TRIGGER < floodCounter) {
			Socket s;
			try {
				s.accept(sock);
			} catch(const SocketException&) {
				// ...
			}
			dcdebug("Connection flood detected!\n");
			return;
		} else {
			floodCounter += FLOOD_ADD;
		}
	}
	UserConnection* uc = getConnection(false, secure);
	uc->setFlag(UserConnection::FLAG_INCOMING);
	uc->setState(UserConnection::STATE_SUPNICK);
	uc->setLastActivity(GET_TICK());
	try { 
		uc->accept(sock);
	} catch(const Exception&) {
		putConnection(uc);
		delete uc;
	}
}

void ConnectionManager::nmdcConnect(const string& aServer, unsigned short aPort, const string& aNick, const string& hubUrl, bool stealth) {
	if(shuttingDown)
		return;

	UserConnection* uc = getConnection(true, false);
	uc->setToken(aNick);
	uc->setHubUrl(hubUrl);
	uc->setState(UserConnection::STATE_CONNECT);
	uc->setFlag(UserConnection::FLAG_NMDC);
	if(stealth) {
		uc->setFlag(UserConnection::FLAG_STEALTH);
	}
	try {
		uc->connect(aServer, aPort);
	} catch(const Exception&) {
		putConnection(uc);
		delete uc;
	}
}

void ConnectionManager::adcConnect(const OnlineUser& aUser, unsigned short aPort, const string& aToken, bool secure) {
	if(shuttingDown)
		return;

	UserConnection* uc = getConnection(false, secure);
	uc->setToken(aToken);
	uc->setState(UserConnection::STATE_CONNECT);
	if(aUser.getIdentity().isOp()) {
		uc->setFlag(UserConnection::FLAG_OP);
	}
	try {
		uc->connect(aUser.getIdentity().getIp(), aPort);
	} catch(const Exception&) {
		putConnection(uc);
		delete uc;
	}
}

void ConnectionManager::disconnect() throw() {
	delete server;
        server = 0; 
#ifdef PPA_INCLUDE_SSL
	delete secureServer;

	server = secureServer = 0;
#endif
}


void ConnectionManager::on(AdcCommand::SUP, UserConnection* aSource, const AdcCommand& cmd) throw() {
	if(aSource->getState() != UserConnection::STATE_SUPNICK) {
		// Already got this once, ignore...@todo fix support updates
		dcdebug("CM::onMyNick %p sent nick twice\n", (void*)aSource);
		return;
	}

	bool baseOk = false;

	for(StringIterC i = cmd.getParameters().begin(); i != cmd.getParameters().end(); ++i) {
		if(i->compare(0, 2, "AD") == 0) {
			string feat = i->substr(2);
			if(feat == UserConnection::FEATURE_ADC_BASE) {
				baseOk = true;
				// ADC clients must support all these...
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_ADCGET);
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_MINISLOTS);
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_TTHF);
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_TTHL);
				// For compatibility with older clients...
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
			} else if(feat == UserConnection::FEATURE_ZLIB_GET) {
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_ZLIB_GET);
			} else if(feat == UserConnection::FEATURE_ADC_BZIP) {
				aSource->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
			}
		}
	}

	if(!baseOk) {
		aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "Invalid SUP"));
		aSource->disconnect();
		return;
	}

	if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
		StringList defFeatures = adcFeatures;
		if(BOOLSETTING(COMPRESS_TRANSFERS)) {
			defFeatures.push_back("AD" + UserConnection::FEATURE_ZLIB_GET);
		}
		aSource->sup(defFeatures);
		aSource->inf(false);
	} else {
		aSource->inf(true);
	}
	aSource->setState(UserConnection::STATE_INF);
}

void ConnectionManager::on(AdcCommand::STA, UserConnection*, const AdcCommand&) throw() {
	
}

void ConnectionManager::on(UserConnectionListener::Connected, UserConnection* aSource) throw() {
#ifdef PPA_INCLUDE_SSL
	if(aSource->isSecure() && !aSource->isTrusted() && !BOOLSETTING(ALLOW_UNTRUSTED_CLIENTS)) {
		putConnection(aSource);
		LogManager::getInstance()->message(STRING(CERTIFICATE_NOT_TRUSTED));
		return;
	}
#endif
	dcassert(aSource->getState() == UserConnection::STATE_CONNECT);
	if (SETTING(GARBAGE_COMMAND_OUTGOING))
		aSource->garbageCommand();
	if(aSource->isSet(UserConnection::FLAG_NMDC)) {
		aSource->myNick(aSource->getToken());
		aSource->lock(CryptoManager::getInstance()->getLock(), CryptoManager::getInstance()->getPk());
	} else {
		StringList defFeatures = adcFeatures;
		if(BOOLSETTING(COMPRESS_TRANSFERS)) {
			defFeatures.push_back("AD" + UserConnection::FEATURE_ZLIB_GET);
		}
		aSource->sup(defFeatures);
	}
	aSource->setState(UserConnection::STATE_SUPNICK);
}

void ConnectionManager::on(UserConnectionListener::MyNick, UserConnection* aSource, const string& aNick) throw() {
	if(aSource->getState() != UserConnection::STATE_SUPNICK) {
		// Already got this once, ignore...
		dcdebug("CM::onMyNick %p sent nick twice\n", (void*)aSource);
		return;
	}

	dcassert(aNick.size() > 0);
//[-]PPA	dcdebug("ConnectionManager::onMyNick %p, %s\n", (void*)aSource, aNick.c_str());
	dcassert(!aSource->getUser());

	if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
		// Try to guess where this came from...
		pair<string, string> i = expectedConnections.remove(aNick);
		if(i.second.empty()) {
			dcassert(i.first.empty());
			dcdebug("Unknown incoming connection from %s\n", aNick.c_str());
			putConnection(aSource);
			return;
		}
        aSource->setToken(i.first);	
		aSource->setHubUrl(i.second);
	}
	CID cid = ClientManager::getInstance()->makeCid(aNick, aSource->getHubUrl());

	// First, we try looking in the pending downloads...hopefully it's one of them...
	{
		Lock l(cs);
		for(ConnectionQueueItem::Iter i = downloads.begin(); i != downloads.end(); ++i) {
			ConnectionQueueItem* cqi = *i;
			if((cqi->getState() == ConnectionQueueItem::CONNECTING || cqi->getState() == ConnectionQueueItem::WAITING) && cqi->getUser()->getCID() == cid) {
				aSource->setUser(cqi->getUser());
				// Indicate that we're interested in this file...
				aSource->setFlag(UserConnection::FLAG_DOWNLOAD);
				break;
			}
		}
	}

	if(!aSource->getUser()) {
		// Make sure we know who it is, i e that he/she is connected...

		aSource->setUser(ClientManager::getInstance()->findUser(cid));
		if(!aSource->getUser() || !ClientManager::getInstance()->isOnline(aSource->getUser())) {
			dcdebug("CM::onMyNick Incoming connection from unknown user %s\n", aNick.c_str());
			putConnection(aSource);
			return;
		}
		// We don't need this connection for downloading...make it an upload connection instead...
		aSource->setFlag(UserConnection::FLAG_UPLOAD);
	}

	ClientManager::getInstance()->setIPUser(aSource->getRemoteIp(), aSource->getUser());

	if(ClientManager::getInstance()->isOp(aSource->getUser(), aSource->getHubUrl()))
		aSource->setFlag(UserConnection::FLAG_OP);

	if(ClientManager::getInstance()->isStealth(aSource->getHubUrl()))
		aSource->setFlag(UserConnection::FLAG_STEALTH);

	if( aSource->isSet(UserConnection::FLAG_INCOMING) ) {
		if(SETTING(GARBAGE_COMMAND_INCOMING) && !aSource->isSet(UserConnection::FLAG_STEALTH))
			aSource->garbageCommand();
		aSource->myNick(aSource->getToken()); 
		aSource->lock(CryptoManager::getInstance()->getLock(), CryptoManager::getInstance()->getPk());
	}

	aSource->setState(UserConnection::STATE_LOCK);
}

void ConnectionManager::on(UserConnectionListener::CLock, UserConnection* aSource, const string& aLock, const string& aPk) throw() {
	if(aSource->getState() != UserConnection::STATE_LOCK) {
		dcdebug("CM::onLock %p received lock twice, ignoring\n", (void*)aSource);
		return;
	}

	if( CryptoManager::getInstance()->isExtended(aLock) ) {
		// Alright, we have an extended protocol, set a user flag for this user and refresh his info...
		if( (aPk.find("DCPLUSPLUS") != string::npos) && aSource->getUser() && !aSource->getUser()->isSet(User::DCPLUSPLUS)) {
			aSource->getUser()->setFlag(User::DCPLUSPLUS);
		}
		StringList defFeatures = features;
		if(BOOLSETTING(COMPRESS_TRANSFERS)) {
			defFeatures.push_back(UserConnection::FEATURE_GET_ZBLOCK);
			defFeatures.push_back(UserConnection::FEATURE_ZLIB_GET);
		}

		aSource->supports(defFeatures);
	}

	aSource->setState(UserConnection::STATE_DIRECTION);
	aSource->direction(aSource->getDirectionString(), aSource->getNumber());
	aSource->key(CryptoManager::getInstance()->makeKey(aLock));

        if(aSource->getUser())
		ClientManager::getInstance()->setPkLock(aSource->getUser(), aPk, aLock);
}

void ConnectionManager::on(UserConnectionListener::Direction, UserConnection* aSource, const string& dir, const string& num) throw() {
	if(aSource->getState() != UserConnection::STATE_DIRECTION) {
		dcdebug("CM::onDirection %p received direction twice, ignoring\n", (void*)aSource);
		return;
	}

	dcassert(aSource->isSet(UserConnection::FLAG_DOWNLOAD) ^ aSource->isSet(UserConnection::FLAG_UPLOAD));
	if(dir == "Upload") {
		// Fine, the other fellow want's to send us data...make sure we really want that...
		if(aSource->isSet(UserConnection::FLAG_UPLOAD)) {
			// Huh? Strange...disconnect...
			putConnection(aSource);
			return;
		}
	} else {
		if(aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
			int number = Util::toInt(num);
			// Damn, both want to download...the one with the highest number wins...
			if(aSource->getNumber() < number) {
				// Damn! We lost!
				aSource->unsetFlag(UserConnection::FLAG_DOWNLOAD);
				aSource->setFlag(UserConnection::FLAG_UPLOAD);
			} else if(aSource->getNumber() == number) {
				putConnection(aSource);
				return;
			}
		}
	}

	dcassert(aSource->isSet(UserConnection::FLAG_DOWNLOAD) ^ aSource->isSet(UserConnection::FLAG_UPLOAD));

	aSource->setState(UserConnection::STATE_KEY);
}

void ConnectionManager::addDownloadConnection(UserConnection* uc) {
	dcassert(uc->isSet(UserConnection::FLAG_DOWNLOAD));
	bool addConn = false;
	{
		Lock l(cs);

		ConnectionQueueItem::Iter i = find(downloads.begin(), downloads.end(), uc->getUser());
		if(i != downloads.end()) {
			ConnectionQueueItem* cqi = *i;
			if(cqi->getState() == ConnectionQueueItem::WAITING || cqi->getState() == ConnectionQueueItem::CONNECTING) {
				cqi->setState(ConnectionQueueItem::ACTIVE);
				uc->setFlag(UserConnection::FLAG_ASSOCIATED);

				fire(ConnectionManagerListener::Connected(), cqi);
				
				dcdebug("ConnectionManager::addDownloadConnection, leaving to downloadmanager\n");
				addConn = true;
			}
		}
	}

	if(addConn) {
		DownloadManager::getInstance()->addConnection(uc);
	} else {
		putConnection(uc);
	}
}

void ConnectionManager::addUploadConnection(UserConnection* uc) {
	dcassert(uc->isSet(UserConnection::FLAG_UPLOAD));

	bool addConn = false;
	{
		Lock l(cs);

		ConnectionQueueItem::Iter i = find(uploads.begin(), uploads.end(), uc->getUser());
		if(i == uploads.end()) {
 		    ConnectionQueueItem* cqi = getCQI(uc->getUser(), false);

				cqi->setState(ConnectionQueueItem::ACTIVE);
				uc->setFlag(UserConnection::FLAG_ASSOCIATED);

				fire(ConnectionManagerListener::Connected(), cqi);

				dcdebug("ConnectionManager::addUploadConnection, leaving to uploadmanager\n");
				addConn = true;
			  }
		}

	if(addConn) {
		UploadManager::getInstance()->addConnection(uc);
	} else {
		putConnection(uc);
	}
}

void ConnectionManager::on(UserConnectionListener::Key, UserConnection* aSource, const string&/* aKey*/) throw() {
	if(aSource->getState() != UserConnection::STATE_KEY) {
		dcdebug("CM::onKey Bad state, ignoring");
		return;
	}

	dcassert(aSource->getUser());

	if(aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
		addDownloadConnection(aSource);
	} else {
		addUploadConnection(aSource);
	}
}

void ConnectionManager::on(AdcCommand::INF, UserConnection* aSource, const AdcCommand& cmd) throw() {
	if(aSource->getState() != UserConnection::STATE_INF) {
		// Already got this once, ignore...
		aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "Expecting INF"));
		dcdebug("CM::onINF %p sent INF twice\n", (void*)aSource);
		aSource->disconnect();
		return;
	}

	string cid;
	if(!cmd.getParam("ID", 0, cid)) {
		aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_INF_MISSING, "ID missing").addParam("FL", "ID"));
		dcdebug("CM::onINF missing ID\n");
		aSource->disconnect();
		return;
	}

	aSource->setUser(ClientManager::getInstance()->findUser(CID(cid)));

	if(!aSource->getUser()) {
		dcdebug("CM::onINF: User not found");
		aSource->send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_GENERIC, "User not found"));
		putConnection(aSource);
		return;
	}

	if(aSource->isSet(UserConnection::FLAG_INCOMING)) {
		aSource->setFlag(UserConnection::FLAG_DOWNLOAD);
		addDownloadConnection(aSource);
	} else {
		aSource->setFlag(UserConnection::FLAG_UPLOAD);
		addUploadConnection(aSource);
	}
}

void ConnectionManager::on(UserConnectionListener::Failed, UserConnection* aSource, const string& aError) throw() {
	Lock l(cs);

	if(aSource->isSet(UserConnection::FLAG_ASSOCIATED)) {
		if(aSource->isSet(UserConnection::FLAG_DOWNLOAD)) {
			ConnectionQueueItem::Iter i = find(downloads.begin(), downloads.end(), aSource->getUser());
			dcassert(i != downloads.end());
			ConnectionQueueItem* cqi = *i;
			cqi->setState(ConnectionQueueItem::WAITING);
			cqi->setLastAttempt((aSource->getLastActivity() == 0) ? 0 : GET_TICK());
			fire(ConnectionManagerListener::Failed(), cqi, aError);
		} else if(aSource->isSet(UserConnection::FLAG_UPLOAD)) {
			ConnectionQueueItem::Iter i = find(uploads.begin(), uploads.end(), aSource->getUser());
			dcassert(i != uploads.end());
			ConnectionQueueItem* cqi = *i;
			putCQI(cqi);
		}
	}
	putConnection(aSource);
}

void ConnectionManager::disconnect(const UserPtr& aUser, int isDownload) {
	Lock l(cs);
	for(UserConnection::Iter i = userConnections.begin(); i != userConnections.end(); ++i) {
		UserConnection* uc = *i;
		if(uc->getUser() == aUser && uc->isSet(isDownload ? UserConnection::FLAG_DOWNLOAD : UserConnection::FLAG_UPLOAD)) {
			uc->disconnect(true);
			break;
		}
	}
}

void ConnectionManager::shutdown() {
	TimerManager::getInstance()->removeListener(this);
	shuttingDown = true;
	disconnect();
	{
		Lock l(cs);
		for(UserConnection::Iter j = userConnections.begin(); j != userConnections.end(); ++j) {
			(*j)->disconnect(true);
		}
	}
	// Wait until all connections have died out...
	while(true) {
		{
			Lock l(cs);
			if(userConnections.empty()) {
				break;
			}
		}
		Thread::sleep(50);
	}
}		

// UserConnectionListener
void ConnectionManager::on(UserConnectionListener::Supports, UserConnection* conn, const StringList& feat) throw() {
	string sup = Util::emptyString;
	for(StringList::const_iterator i = feat.begin(); i != feat.end(); ++i) {
		sup += (string)*i + " ";
		if(*i == UserConnection::FEATURE_GET_ZBLOCK) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_GETZBLOCK);
		} else if(*i == UserConnection::FEATURE_MINISLOTS) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_MINISLOTS);
		} else if(*i == UserConnection::FEATURE_XML_BZLIST) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_XML_BZLIST);
		} else if(*i == UserConnection::FEATURE_ADCGET) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_ADCGET);
		} else if(*i == UserConnection::FEATURE_ZLIB_GET) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_ZLIB_GET);
		} else if(*i == UserConnection::FEATURE_TTHL) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_TTHL);
                } else if(*i == UserConnection::FEATURE_BANMSG) { // !SMT!-S
                        conn->setFlag(UserConnection::FLAG_SUPPORTS_BANMSG); // !SMT!-S
		} else if(*i == UserConnection::FEATURE_TTHF) {
			conn->setFlag(UserConnection::FLAG_SUPPORTS_TTHF);
			if(conn->getUser()) 
				conn->getUser()->setFlag(User::TTH_GET);
		}
	}

	if(conn->getUser())
		ClientManager::getInstance()->setSupports(conn->getUser(), sup);
}

// !SMT!-S
void ConnectionManager::setUploadLimit(const UserPtr& aUser, FavoriteUser::UPLOAD_LIMIT lim)
{
   Lock l(cs);
   for (UserConnection::Iter i = userConnections.begin(); i != userConnections.end(); ++i)
      if((*i)->getUser() == aUser && (*i)->isSet(UserConnection::FLAG_UPLOAD))
         (*i)->setUploadLimit(lim);
}



/**
 * @file
 * $Id: ConnectionManager.cpp,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
