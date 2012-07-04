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

#if !defined(CONNECTION_MANAGER_H)
#define CONNECTION_MANAGER_H

#include "TimerManager.h"

#include "UserConnection.h"
#include "User.h"
#include "CriticalSection.h"
#include "Singleton.h"
#include "Util.h"

#include "ConnectionManagerListener.h"

class SocketException;

class ConnectionQueueItem {
public:
	typedef ConnectionQueueItem* Ptr;
	typedef FlyLinkVector<Ptr> List;
	typedef List::const_iterator Iter;
	
	enum State {
		CONNECTING,					// Recently sent request to connect
		WAITING,					// Waiting to send request to connect
		NO_DOWNLOAD_SLOTS,			// Not needed right now
		ACTIVE						// In one up/downmanager
	};

	ConnectionQueueItem(const UserPtr& aUser, bool aDownload) : state(WAITING), lastAttempt(0), download(aDownload), token(Util::toString(Util::rand())), user(aUser) { }
	
	UserPtr& getUser() { return user; }
	const UserPtr& getUser() const { return user; }
	
	GETSET(State, state, State);
	GETSET(uint64_t, lastAttempt, LastAttempt);
	GETSET(bool, download, Download);
	GETSET(string, token, Token);
private:
	ConnectionQueueItem(const ConnectionQueueItem&);
	ConnectionQueueItem& operator=(const ConnectionQueueItem&);
	
	UserPtr user;
};

class ExpectedMap {
public:
	void add(const string& aNick, const string& aMyNick, const string& aHubUrl) {
		Lock l(cs);
		expectedConnections.insert(make_pair(aNick, make_pair(aMyNick, aHubUrl)));
	}

	StringPair remove(const string& aNick) {
		Lock l(cs);
		ExpectMap::iterator i = expectedConnections.find(aNick);
		
		if(i == expectedConnections.end()) 
			return make_pair(Util::emptyString, Util::emptyString);

		StringPair tmp = i->second;
		expectedConnections.erase(i);
		
		return tmp;
	}

private:
	/** Nick -> myNick, hubUrl for expected NMDC incoming connections */
	typedef map<string, StringPair> ExpectMap;
	ExpectMap expectedConnections;

	CriticalSection cs;
};

// Comparing with a user...
inline bool operator==(ConnectionQueueItem::Ptr ptr, const UserPtr& aUser) { return ptr->getUser() == aUser; }

class ConnectionManager : public Speaker<ConnectionManagerListener>, 
	public UserConnectionListener, TimerManagerListener, 
	public Singleton<ConnectionManager>
{
public:
	static bool m_DisableAutoBan; //[+]PPA
	void nmdcExpect(const string& aNick, const string& aMyNick, const string& aHubUrl) {
		expectedConnections.add(aNick, aMyNick, aHubUrl);
	}

	void nmdcConnect(const string& aServer, uint16_t aPort, const string& aMyNick, const string& hubUrl, bool stealth);
	void adcConnect(const OnlineUser& aUser, uint16_t aPort, const string& aToken, bool secure);

	void getDownloadConnection(const UserPtr& aUser);
        void setUploadLimit(const UserPtr& aUser, FavoriteUser::UPLOAD_LIMIT lim);
	
	void disconnect(const UserPtr& aUser, int isDownload);

	void shutdown();
	bool isShuttingDown() const { return shuttingDown; }

	/** Find a suitable port to listen on, and start doing it */
	void listen() throw(SocketException);
	void disconnect() throw();

	uint16_t getPort() const { return server ? static_cast<unsigned short>(server->getPort()) : 0; }
#ifdef PPA_INCLUDE_SSL
	uint16_t getSecurePort() const { return secureServer ? static_cast<unsigned short>(secureServer->getPort()) : 0; 
	                               }
#else
	uint16_t getSecurePort() const { return 0;
	                               }
#endif
private:

	class Server : public Thread {
	public:
		Server(bool secure_, uint16_t port, const string& ip = "0.0.0.0");
		uint16_t getPort() { return port; }
		~Server() { die = true; join(); }
	private:
		int run() throw();

		Socket sock;
		uint16_t port;
#ifdef PPA_INCLUDE_SSL
		bool secure;
#endif
		bool die;
	};

	friend class Server;

	CriticalSection cs;

	/** All ConnectionQueueItems */
	ConnectionQueueItem::List downloads;
	ConnectionQueueItem::List uploads;

	/** All active connections */
	UserConnection::List userConnections;

	UserList checkIdle;

	StringList features;
	StringList adcFeatures;

	ExpectedMap expectedConnections;

	uint64_t floodCounter;

	Server* server;
#ifdef PPA_INCLUDE_SSL
	Server* secureServer;
#endif
	bool shuttingDown;

	friend class Singleton<ConnectionManager>;
	ConnectionManager();

	~ConnectionManager() throw() { shutdown(); }
	
	UserConnection* getConnection(bool aNmdc, bool secure) throw();
	void putConnection(UserConnection* aConn);

	void addUploadConnection(UserConnection* uc);
	void addDownloadConnection(UserConnection* uc);

	ConnectionQueueItem* getCQI(const UserPtr& aUser, bool download);
	void putCQI(ConnectionQueueItem* cqi);

	void accept(const Socket& sock, bool secure) throw();

	// UserConnectionListener
	void on(Connected, UserConnection*) throw();
	void on(Failed, UserConnection*, const string&) throw();
	void on(CLock, UserConnection*, const string&, const string&) throw();
	void on(Key, UserConnection*, const string&) throw();
	void on(Direction, UserConnection*, const string&, const string&) throw();
	void on(MyNick, UserConnection*, const string&) throw();
	void on(Supports, UserConnection*, const StringList&) throw();

	void on(AdcCommand::SUP, UserConnection*, const AdcCommand&) throw();
	void on(AdcCommand::INF, UserConnection*, const AdcCommand&) throw();
	void on(AdcCommand::STA, UserConnection*, const AdcCommand&) throw();

	// TimerManagerListener
	void on(TimerManagerListener::Second, uint32_t aTick) throw();	
	void on(TimerManagerListener::Minute, uint32_t aTick) throw();	

};

#endif // !defined(CONNECTION_MANAGER_H)

/**
 * @file
 * $Id: ConnectionManager.h,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
