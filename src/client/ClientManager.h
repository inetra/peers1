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

#if !defined(CLIENT_MANAGER_H)
#define CLIENT_MANAGER_H

//[-]PPA [Doxygen 1.5.1] #include "TimerManager.h"

#include "Client.h"
//[-]PPA [Doxygen 1.5.1] #include "Singleton.h"
#include "SettingsManager.h"
#include "DirectoryListing.h"

#include "ClientManagerListener.h"

class UserCommand;

class ClientManager : public Speaker<ClientManagerListener>, 
	private ClientListener, public ParametrizableSingleton<ClientManager,string>, 
	private TimerManagerListener, private SettingsManagerListener
{
public:
	Client* getClient(const string& aHubURL);
	void putClient(Client* aClient);
	Client* findClient(const string& aUrl) const; // !SMT!-S

	size_t getUserCount() const;
	int64_t getAvailable() const;
	StringList getHubs(const CID& cid) const;
	StringList getHubNames(const CID& cid) const;
	//StringList getNicks(const CID& cid) const;
	string getConnection(const CID& cid) const;

	bool isConnected(const string& aUrl) const;
	
	void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	void search(StringList& who, int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	void infoUpdated(bool antispam);

	UserPtr getUser(const string& aNick, const string& aHubUrl) throw();
	UserPtr getUser(const CID& cid) throw();

	string findHub(const string& ipPort) const;

	UserPtr findUser(const string& aNick, const string& aHubUrl) const throw() { return findUser(makeCid(aNick, aHubUrl)); }
	UserPtr findUser(const CID& cid) const throw();
	UserPtr findLegacyUser(const string& aNick) const throw();

	bool isOnline(const UserPtr& aUser) const {
		Lock l(cs);
		return onlineUsers.find(aUser->getCID()) != onlineUsers.end();
	}
	
        // !SMT!-IP
        void logUserIp(const string& IP, const UserPtr& user);
        typedef set<string> MapLoggedUserIps;
        typedef MapLoggedUserIps::const_iterator LogIter;
        MapLoggedUserIps loggedUserIps;
        bool m_GuardOpenFile; //[+]PPA
        // end !SMT!-IP

	void setIPUser(const string& IP, const UserPtr& user) {
		Lock l(cs);
		OnlinePairC p = onlineUsers.equal_range(user->getCID());
		for (OnlineIterC i = p.first; i != p.second; i++) i->second->getIdentity().setIp(IP);
                logUserIp(IP, user); // !SMT!-IP
	}	
	
	const string getMyNMDCNick(const UserPtr& p) const {
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			return i->second->getClient().getMyNick();
		}
		return Util::emptyString;
	}

	bool getSharingHub(const UserPtr& p) {
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i != onlineUsers.end()) {
			return i->second->getClient().getHideShare();
		}
		return false;
	}

	void reportUser(const UserPtr& p) {
		string nick; string report;
		Client* c;
		{
			Lock l(cs);
			OnlineIterC i = onlineUsers.find(p->getCID());
			if(i == onlineUsers.end()) return;

			nick = i->second->getIdentity().getNick();
			report = i->second->getIdentity().getReport();
			c = &i->second->getClient();
		}
		c->cheatMessage("*** Info on " + nick + " ***" + "\r\n" + report + "\r\n");
	}

	// !SMT!-S
        OnlineUser* getOnlineUser(const UserPtr& p) {
                // Lock l(cs);  - getOnlineUser() return value should be gaurded with ClientManager::lock(),unlock
                if (p == (UserPtr)NULL) return NULL;
                OnlineIterC i = onlineUsers.find(p->getCID());
                if(i == onlineUsers.end()) return NULL;
                return i->second;
        }
        UserPtr getUserByIp(const string &ip) {
                Lock l(cs);
                for(OnlineIterC i = onlineUsers.begin(); i != onlineUsers.end(); ++i) {
                        if (i->second->getIdentity().getIp() == ip)
                                return i->second->getUser();
                }
                return NULL;
        }
        // !SMT!-S
        bool getIdentity(const UserPtr& user, Identity &id) { 
                Lock l(cs);
                OnlineUser *ou = getOnlineUser(user);
                if (ou) 
					id = ou->getIdentity();
                else 
					id = Identity(NULL, 0);
                return (ou != NULL);
        }
   void setPkLock(const UserPtr& p, const string& aPk, const string& aLock) {
                Client *c; // !SMT!-fix
                {
                        Lock l(cs);
                        OnlineIterC i = onlineUsers.find(p->getCID());
                        if(i == onlineUsers.end()) return;

                        OnlineUser* ou = i->second;
                        ou->getIdentity().set("PK", aPk);
                        ou->getIdentity().set("LO", aLock);
                        c = &(ou->getClient()); // !SMT!-fix
                }
                c->updated(p); // !SMT!-fix
        }

	void setSupports(const UserPtr& p, const string& aSupports) {
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;
		i->second->getIdentity().set("SU", aSupports);
	}

	void setGenerator(const UserPtr& p, const string& aGenerator) {
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;
		i->second->getIdentity().set("GE", aGenerator);
	}

	void setUnknownCommand(const UserPtr& p, const string& aUnknownCommand) {
		Lock l(cs);
		OnlineIterC i = onlineUsers.find(p->getCID());
		if(i == onlineUsers.end()) return;
		i->second->getIdentity().set("UC", aUnknownCommand);
	}

	bool isOp(const UserPtr& aUser, const string& aHubUrl) const;
	bool isStealth(const string& aHubUrl) const;

	/** Constructs a synthetic, hopefully unique CID */
	CID makeCid(const string& nick, const string& hubUrl) const throw();

	void putOnline(OnlineUser* ou) throw();
	void putOffline(OnlineUser* ou) throw();

	UserPtr& getMe();
	
	void connect(const UserPtr& p);
	void send(AdcCommand& c, const CID& to);
        void privateMessage(const UserPtr& p, const string& msg, bool annoying = true); // !SMT!-S

	void userCommand(const UserPtr& p, const ::UserCommand& uc, StringMap& params, bool compatibility);

	int getMode(const string& aHubUrl);
	bool isActive(const string& aHubUrl) { return getMode(aHubUrl) != SettingsManager::INCOMING_FIREWALL_PASSIVE; }

        // !PPA!
	class LockInstance	{
		ClientManager* m_instance;
	public:
		LockInstance(ClientManager* anInstance = NULL) {
			m_instance = anInstance? anInstance : getInstance();
			m_instance->cs.enter();
		}
		~LockInstance() {
			m_instance->cs.leave();
		}
		ClientManager* operator->()	{
			return m_instance;
		}
	};

	const string& getHubUrl(const UserPtr& aUser) const;

	Client::List& getClients() { return clients; }

	CID getMyCID();
	const CID& getMyPID();
	
	// fake detection methods
	void setListLength(const UserPtr& p, const string& listLen);
	void fileListDisconnected(const UserPtr& p);
	void connectionTimeout(const UserPtr& p);
	void checkCheating(const UserPtr& p, DirectoryListing* dl);
	void setCheating(const UserPtr& p, const string& aTestSURString, const string& aCheatString, const int aRawCommand, bool aBadClient);
	void setFakeList(const UserPtr& p, const string& aCheatString);

private:
	typedef HASH_MAP_X(CID, UserPtr, CID::Hash, equal_to<CID>, less<CID>) UserMap;
	typedef UserMap::iterator UserIter;

	typedef HASH_MULTIMAP_X(CID, OnlineUser*, CID::Hash, equal_to<CID>, less<CID>) OnlineMap;
	typedef OnlineMap::iterator OnlineIter;
	typedef OnlineMap::const_iterator OnlineIterC;
	typedef pair<OnlineIter, OnlineIter> OnlinePair;
	typedef pair<OnlineIterC, OnlineIterC> OnlinePairC;

	Client::List clients;
	mutable CriticalSection cs;
	
	UserMap users;
	OnlineMap onlineUsers;

	UserPtr me;
	
	uint64_t quickTick;
	
	CID pid;	

	friend class ParametrizableSingleton<ClientManager,string>;

        string m_clientId;

        ClientManager(const string& clientId):m_clientId(clientId) { 
		TimerManager::getInstance()->addListener(this); 
		SettingsManager::getInstance()->addListener(this);
		quickTick = GET_TICK();
		m_GuardOpenFile = false; //[+]PPA
	}

	virtual ~ClientManager() throw() { 
		SettingsManager::getInstance()->removeListener(this);
		TimerManager::getInstance()->removeListener(this); 
	}


	// SettingsManagerListener
	virtual void on(Load, SimpleXML&) throw();

	// ClientListener
	virtual void on(Connected, Client* c) throw() { fire(ClientManagerListener::ClientConnected(), c); }
        virtual void on(UserUpdated, Client*, const UserPtr& user) throw() { fire(ClientManagerListener::UserUpdated(), user); } // !SMT!-fix
	virtual void on(UsersUpdated, Client* c, const UserList&) throw() { fire(ClientManagerListener::ClientUpdated(), c); }
	virtual void on(Failed, Client*, const string&) throw();
	virtual void on(HubUpdated, Client* c) throw() { fire(ClientManagerListener::ClientUpdated(), c); }
	virtual void on(UserCommand, Client*, int, int, const string&, const string&) throw();
	virtual void on(NmdcSearch, Client* aClient, const string& aSeeker, int aSearchType, int64_t aSize, 
		int aFileType, const string& aString, bool) throw();
	virtual void on(AdcSearch, Client* c, const AdcCommand& adc, const CID& from) throw();
	// TimerManagerListener
	virtual void on(TimerManagerListener::Minute, uint32_t aTick) throw();
};

#endif // !defined(CLIENT_MANAGER_H)

/**
 * @file
 * $Id: ClientManager.h,v 1.7 2008/12/20 04:25:05 alexey Exp $
 */
