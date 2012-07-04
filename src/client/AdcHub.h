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

#if !defined(ADC_HUB_H)
#define ADC_HUB_H


#include "Client.h"
#include "AdcCommand.h"
#include "TimerManager.h"
//[-]PPA [Doxygen 1.5.1] #include "User.h"

class ClientManager;

class AdcHub : public Client, public CommandHandler<AdcHub> {
public:
	using Client::send;
	using Client::connect;

	virtual void connect(const OnlineUser& user);
	void connect(const OnlineUser& user, string const& token, bool secure);
	
	virtual void hubMessage(const string& aMessage);
        virtual void privateMessage(const UserPtr& user, const string& aMessage, bool annoying = true); // !SMT!-S
	virtual void sendUserCmd(const string& aUserCmd) { send(aUserCmd); }
	virtual void search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	virtual void password(const string& pwd);
	virtual void info();
	virtual void cheatMessage(const string&) { } 

	virtual size_t getUserCount() const { Lock l(cs); return users.size(); }

        //virtual OnlineUser* findUser(const string& aNick); // !SMT!-S
        virtual string escape(string const& str) const { return AdcCommand::escape(str, false); }
        virtual void send(const AdcCommand& cmd);
        void refreshUserList(bool) { }

	string getMySID() { return AdcCommand::fromSID(sid); }
private:
	friend class ClientManager;
	friend class CommandHandler<AdcHub>;

	AdcHub(const string &clientId, const string& aHubURL, bool secure);

	AdcHub(const AdcHub&);
	AdcHub& operator=(const AdcHub&);
	virtual ~AdcHub() throw();

	/** Map session id to OnlineUser */
	typedef HASH_MAP<uint32_t, OnlineUser*> SIDMap;
	typedef SIDMap::iterator SIDIter;

	Socket udp;
	SIDMap users;
	StringMap lastInfoMap;
	mutable CriticalSection cs;

	string salt;
	uint32_t sid;

	static const string CLIENT_PROTOCOL;
	static const string SECURE_CLIENT_PROTOCOL;
	static const string ADCS_FEATURE;
	static const string TCP4_FEATURE;
	static const string UDP4_FEATURE;
	 
	virtual string checkNick(const string& nick);

	OnlineUser& getUser(const uint32_t aSID, const CID& aCID);
	OnlineUser* findUser(const uint32_t sid) const;
	OnlineUser* findUser(const CID& cid) const;
	void putUser(const uint32_t sid);

	void clearUsers();

	void handle(AdcCommand::SUP, AdcCommand& c) throw();
	void handle(AdcCommand::SID, AdcCommand& c) throw();
	void handle(AdcCommand::MSG, AdcCommand& c) throw();
	void handle(AdcCommand::INF, AdcCommand& c) throw();
	void handle(AdcCommand::GPA, AdcCommand& c) throw();
	void handle(AdcCommand::QUI, AdcCommand& c) throw();
	void handle(AdcCommand::CTM, AdcCommand& c) throw();
	void handle(AdcCommand::RCM, AdcCommand& c) throw();
	void handle(AdcCommand::STA, AdcCommand& c) throw();
	void handle(AdcCommand::SCH, AdcCommand& c) throw();
	void handle(AdcCommand::CMD, AdcCommand& c) throw();
	void handle(AdcCommand::RES, AdcCommand& c) throw();

	template<typename T> void handle(T, AdcCommand&) { }

	void sendUDP(const AdcCommand& cmd) throw();

	virtual void on(Connecting) throw() { fire(ClientListener::Connecting(), this); }
	virtual void on(Connected) throw();
	virtual void on(Line, const string& aLine) throw();
	virtual void on(Failed, const string& aLine) throw();

	virtual void on(Second, uint32_t aTick) throw();

};

#endif // !defined(ADC_HUB_H)

/**
 * @file
 * $Id: AdcHub.h,v 1.2 2007/10/15 10:54:30 alexey Exp $
 */
