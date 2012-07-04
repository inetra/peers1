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

#if !defined(NMDC_HUB_H)
#define NMDC_HUB_H

#include "TimerManager.h"
#include "SettingsManager.h"

#include "User.h"
#include "CriticalSection.h"
#include "Text.h"
//[-]PPA [Doxygen 1.5.1] #include "Client.h"
#include "ConnectionManager.h"
#include "UploadManager.h"
#include "StringTokenizer.h"
#include "ZUtils.h"

class ClientManager;

class NmdcHub : public Client, private Flags
{
public:
	using Client::send;
	using Client::connect;

	void connect(const OnlineUser& aUser);

	void hubMessage(const string& aMessage);
	void privateMessage(const UserPtr& aUser, const string& aMessage, bool annoying = true); // !SMT!-S
	void sendUserCmd(const string& aUserCmd) throw() { send(toAcp(aUserCmd)); }
	void search(int aSizeType, int64_t aSize, int aFileType, const string& aString, const string& aToken);
	void password(const string& aPass) { send("$MyPass " + toAcp(aPass) + "|"); }
	void info() { myInfo(); }

	void cheatMessage(const string& aLine) {
		fire(ClientListener::CheatMessage(), this, unescape(aLine));
	}    

	size_t getUserCount() const {  Lock l(cs); return users.size(); }
	OnlineUser* getUser(const UserPtr& aUser); // !SMT!-S

	string escape(string const& str) const { return validateMessage(str, false); }
	static string unescape(const string& str) { return validateMessage(str, true); }

	void send(const AdcCommand&) { dcassert(0); }

	static string validateMessage(string tmp, bool reverse);
	void refreshUserList(bool);
	
	virtual bool isHubConnected() const { return getState() == STATE_NORMAL && m_protocolState == PS_DONE; }
	
private:
	friend class ClientManager;
	enum SupportFlags {
		SUPPORTS_USERCOMMAND = 0x01,
		SUPPORTS_NOGETINFO = 0x02,
		SUPPORTS_USERIP2 = 0x04
	};	
	enum ProtocolState {
		PS_UNKNOWN,
		PS_WAITING_USER_LIST,
		PS_DONE
	};

	ProtocolState m_protocolState;
	time_t m_waitUserListStarted;

	mutable CriticalSection cs;

	typedef HASH_MAP_X(string, OnlineUser*, noCaseStringHash, noCaseStringEq, noCaseStringLess) NickMap;
	typedef NickMap::const_iterator NickIter;

	NickMap users;

	int supportFlags;
	string lastmyinfo;
	int64_t lastbytesshared;

	typedef list<pair<string, tick_t> > FloodMap;
	typedef FloodMap::const_iterator FloodIter;
	FloodMap seekers;
	FloodMap flooders;

	NmdcHub(const string& clientId, const string& aHubURL);
	~NmdcHub() throw();

	// Dummy
	NmdcHub(const NmdcHub&);
	NmdcHub& operator=(const NmdcHub&);

	void clearUsers();
	void onLine(const string& aLine) throw();

	OnlineUser& getUser(const string& aNick);
	OnlineUser* findUser(const string& aNick);
	void putUser(const string& aNick);
	
	string fromAcp(const string& str) const { return Text::acpToUtf8(str); }
	string toAcp(const string& str) const { return Text::utf8ToAcp(str); }

	void validateNick(const string& aNick) { send("$ValidateNick " + toAcp(aNick) + "|"); }
	void key(const string& aKey) { send("$Key " + aKey + "|"); }
	void version() { send("$Version 1,0091|"); }
	void getNickList() { send("$GetNickList|"); }
	void connectToMe(const OnlineUser& aUser);
	void revConnectToMe(const OnlineUser& aUser);
	void myInfo();
	void supports(const StringList& feat);
	void clearFlooders(tick_t tick);

	void updateFromTag(Identity& id, const string& tag);

	string checkNick(const string& aNick);

	// TimerManagerListener
	void on(Second, uint32_t aTick) throw();

	void on(Connected) throw();
	void on(Line, const string& l) throw();
	void on(Failed, const string&) throw();

};

#endif // !defined(NMDC_HUB_H)

/**
 * @file
 * $Id: nmdchub.h,v 1.3 2008/04/24 16:14:36 alexey Exp $
 */
