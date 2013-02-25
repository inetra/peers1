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

#if !defined(CLIENT_H)
#define CLIENT_H

#include "User.h"
#include "BufferedSocket.h"
#include "SettingsManager.h"
//[-]PPA [Doxygen 1.5.1] #include "TimerManager.h"
#include "DebugManager.h"
#include "ClientListener.h"

/** Yes, this should probably be called a Hub */
class Client : public Speaker<ClientListener>, public BufferedSocketListener, protected TimerManagerListener {
public:
  enum States {
    STATE_CONNECTING,	///< Waiting for socket to connect
    STATE_PROTOCOL,		///< Protocol setup
    STATE_IDENTIFY,		///< Nick setup
    STATE_VERIFY,		///< Checking password
    STATE_NORMAL,		///< Running
    STATE_DISCONNECTED	///< Nothing in particular
  };

	typedef Client* Ptr;
	typedef slist<Ptr> List;
	typedef List::const_iterator Iter;

	virtual void connect();
	virtual void disconnect(bool graceless);

	virtual void connect(const OnlineUser& user) = 0;
	virtual void hubMessage(const string& aMessage) = 0;
        virtual void privateMessage(const UserPtr& user, const string& aMessage, bool annoying = true) = 0; // !SMT!-S
	virtual void sendUserCmd(const string& aUserCmd) = 0;
	virtual void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken) = 0;
	virtual void password(const string& pwd) = 0;
	virtual void info() = 0;
	virtual void cheatMessage(const string& aLine) = 0;

	virtual size_t getUserCount() const = 0;
	int64_t getAvailable() const { return availableBytes; };
        virtual OnlineUser* getUser(const UserPtr& aUser); // !SMT!-S
        //virtual OnlineUser* findUser(const string& aNick) = 0; // !SMT!-S

        virtual void send(const AdcCommand& command) = 0;

	virtual string escape(string const& str) const { return str; }

	bool isConnected() const { return socket && socket->isConnected(); }
	bool isOp() const { return getMyIdentity().isOp(); }

	virtual void refreshUserList(bool) = 0;

	uint16_t getPort() const { return port; }
	const string& getAddress() const { return address; }

	const string& getIp() const { return ip; }
	string getIpPort() const { return getIp() + ':' + Util::toString(port); }
	string getLocalIp() const;

        void updated(const UserPtr& aUser) { fire(ClientListener::UserUpdated(), this, aUser); } // !SMT!-fix

	static int getTotalCounts() {
		return counts.normal + counts.registered + counts.op;
	}

	static string getCounts() {
		char buf[128];
		return string(buf, snprintf(buf, sizeof(buf), "%ld/%ld/%ld", counts.normal, counts.registered, counts.op));
	}

	const string& getRawCommand(const int aRawCommand) const {
		switch(aRawCommand) {
			case 1: return rawOne;
			case 2: return rawTwo;
			case 3: return rawThree;
			case 4: return rawFour;
			case 5: return rawFive;
		}
		return Util::emptyString;
	}
	
	StringMap& escapeParams(StringMap& sm) {
		for(StringMapIter i = sm.begin(); i != sm.end(); ++i) {
			i->second = escape(i->second);
		}
		return sm;
	}

	void reconnect();
	void shutdown();
	// активный режим подключения
	bool isActive() const;
	// состояние клиента - см. enum выше
	States getState() const { return state; }
	virtual bool isHubConnected() const { return getState() == STATE_NORMAL; }

	void send(const string& aMessage) { send(aMessage.c_str(), aMessage.length()); }
	void send(const char* aMessage, size_t aLen) {
		if(!socket)
			return;
		updateActivity();
		socket->write(aMessage, aLen);
		COMMAND_DEBUG(aMessage, DebugManager::HUB_OUT, getIpPort());
	}

	string getMyNick() const { return getMyIdentity().getNick(); }
	string getHubName() const { return getHubIdentity().getNick().empty() ? getHubUrl() : getHubIdentity().getNick(); }
	string getHubDescription() const { return getHubIdentity().getDescription(); }

	Identity& getHubIdentity() { return hubIdentity; }

	const string& getHubUrl() const { return originalHubUrl; }

	GETSET(Identity, myIdentity, MyIdentity);
	GETSET(Identity, hubIdentity, HubIdentity);

	GETSET(string, defpassword, Password);
	GETSET(uint32_t, reconnDelay, ReconnDelay);
	GETSET(tick_t, lastActivity, LastActivity);
	GETSET(bool, registered, Registered);
	GETSET(bool, autoReconnect, AutoReconnect);
	
	GETSET(string, currentNick, CurrentNick);
	GETSET(string, currentDescription, CurrentDescription);
	GETSET(string, currentEmail, CurrentEmail);
		
	GETSET(bool, stealth, Stealth);
	GETSET(bool, hideShare, HideShare); // Hide Share Mod
	GETSET(string, rawOne, RawOne);
	GETSET(string, rawTwo, RawTwo);
	GETSET(string, rawThree, RawThree);
	GETSET(string, rawFour, RawFour);
	GETSET(string, rawFive, RawFive);
	GETSET(string, favIp, FavIp);
        GETSET(string, clientId, ClientId); // !SMT!-S
protected:
	friend class ClientManager;
	Client(const string& clientId, const string& hubURL, char separator, bool secure_);
	virtual ~Client() throw();
	struct Counts {
		Counts(long n = 0, long r = 0, long o = 0) : normal(n), registered(r), op(o) { }
		volatile long normal;
		volatile long registered;
		volatile long op;
		bool operator !=(const Counts& rhs) { return normal != rhs.normal || registered != rhs.registered || op != rhs.op; }
	};

	States state;

	BufferedSocket* socket;

	static Counts counts;
	Counts lastCounts;
public:
	int64_t availableBytes;
protected:

	void updateCounts(bool aRemove);
	void updateActivity() { lastActivity = GET_TICK(); }

	/** Reload details from favmanager or settings */
	void reloadSettings(bool updateNick);

	virtual string checkNick(const string& nick) = 0;

	// TimerManagerListener
	virtual void on(Second, uint32_t aTick) throw();
	// BufferedSocketListener
	virtual void on(Connecting) throw() { fire(ClientListener::Connecting(), this); }
	virtual void on(Connected) throw();
	virtual void on(Line, const string& aLine) throw();
	virtual void on(Failed, const string&) throw();

	virtual void redirectToURL(const std::string& redirectUrl) throw();
private:

	enum CountType {
		COUNT_UNCOUNTED,
		COUNT_NORMAL,
		COUNT_REGISTERED,
		COUNT_OP
	};

	Client(const Client&);
	Client& operator=(const Client&);

	CountType countType;
	string hubUrl;
	string originalHubUrl;
	string address;
	string ip;
        string m_clientId;
	uint16_t port;
	char separator;
	bool secure;
};

#endif // !defined(CLIENT_H)

/**
 * @file
 * $Id: Client.h,v 1.3 2007/11/02 04:55:13 alexey Exp $
 */
