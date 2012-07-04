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

#ifndef DCPLUSPLUS_CLIENT_USER_H
#define DCPLUSPLUS_CLIENT_USER_H

#include "forward.h"
#include "Util.h"
#include "Pointer.h"
#include "CID.h"
#include "FastAlloc.h"
//[-]PPA [Doxygen 1.5.1]  #include "CriticalSection.h"
#include "SettingsManager.h" // !SMT!-S

/** A user connected to one or more hubs. */
class User : public FastAlloc<User>, public PointerBase, public Flags
{
private:
	enum Bits {
		ONLINE_BIT,
		DCPLUSPLUS_BIT,
		PASSIVE_BIT,
		NMDC_BIT,
		BOT_BIT,
		TTH_GET_BIT,
		TLS_BIT,
		OLD_CLIENT_BIT,		
                PG_BLOCK_BIT,
		AWAY_BIT,
		SERVER_BIT,
		FIREBALL_BIT
	};
public:

	/** Each flag is set if it's true in at least one hub */
	enum UserFlags {
		ONLINE = 1<<ONLINE_BIT,
		DCPLUSPLUS = 1<<DCPLUSPLUS_BIT,
		PASSIVE = 1<<PASSIVE_BIT,
		NMDC = 1<<NMDC_BIT,
		BOT = 1<<BOT_BIT,
		TTH_GET = 1<<TTH_GET_BIT,		//< User supports getting files by tth -> don't have path in queue...
		TLS = 1<<TLS_BIT,				//< Client supports TLS
		OLD_CLIENT = 1<<OLD_CLIENT_BIT, //< Can't download - old client
		PG_BLOCK = 1 <<PG_BLOCK_BIT,	//< Blocked by PeerGuardian
		AWAY = 1 << AWAY_BIT,
		SERVER = 1 << SERVER_BIT,
		FIREBALL = 1 << FIREBALL_BIT
	};
	bool hasAutoBan() const;

	typedef vector<UserPtr> PtrList; // !SMT!-S

	struct HashFunction {
		size_t operator()(const UserPtr& x) const { return ((size_t)(&(*x)))/sizeof(User); }
	};

	User(const CID& aCID) : cid(aCID),
		lastDownloadSpeed(0),
		CountSlots(0), 
		BytesShared(0), 
		Limit(0)
	 {
	 }

	virtual ~User() throw() { }

	const CID& getCID() const { return cid; }
	operator const CID&() const { return cid; }

	bool isOnline() const { return isSet(ONLINE); }
	bool isNMDC() const { return isSet(NMDC); }

	GETSET(string, firstNick, FirstNick);
	GETSET(string, lastHubName, LastHubName);	// used in PrivateFrame
	GETSET(uint16_t, lastDownloadSpeed, LastDownloadSpeed);
//[+]PPA
	GETSET(int, Limit, Limit);
	GETSET(int64_t, BytesShared, BytesShared);
	GETSET(int, CountSlots, CountSlots);

private:
	User(const User&);
	User& operator=(const User&);
	CID cid;
};

class Client;
class OnlineUser;

/** One of possibly many identities of a user, mainly for UI purposes */
class Identity {
	inline void _init(const Identity& rhs)
	{
	  user = rhs.user; 
	  m_info_map = rhs.m_info_map; 
	  m_info_bitmap = rhs.m_info_bitmap;
	}
public:
	Identity():m_info_bitmap(0)  {
	}
	Identity(const UserPtr& ptr, uint32_t aSID) : user(ptr),m_info_bitmap(0)  {
      setSID(aSID);
	}
	Identity(const Identity& rhs)
	{ 
	  Lock l(rhs.cs); 
      _init(rhs);  
	}
	Identity& operator=(const Identity& rhs) 
	{ 
	  Lock l2(rhs.cs); 
 	  Lock l1(cs); 
      _init(rhs);  
	  return *this; 
	}
	~Identity() 
	{
		Lock l(cs); 
		m_info_map.clear(); /* don't allow destroying before all critical sections are left */ 
	}

#define GS(n, x) string get##n() const { return get(x); } void set##n(const string& v) { set(x, v); }
	GS(Description, "DE")
	GS(Ip, "I4")
	GS(UdpPort, "U4")
	GS(Email, "EM")
	GS(Status, "ST")
	GS(SS, "SS")
	GS(Nick, "NI")
	
/* [!] PPA не пашет кик из чата
void setNick(const string& aNick) {
		if(!user || !user->isSet(User::NMDC)) {
			set("NI", aNick);
		}
	}

	const string& getNick() const {
		if(user && user->isSet(User::NMDC)) {
			return user->getFirstNick();
		} else {
	            Lock l(cs);
	            InfMap::const_iterator i = m_info_map.find(*(short*)"NI");
	            return i == m_info_map.end() ? Util::emptyString : i->second;
		}
	}
*/
	int64_t getBytesShared() const { return Util::toInt64(get("SS")); }
	
	void setConnection(const string& name) { set("US", name); }
	string getConnection() const;

    void setSlots(const string& bs) { set("SL", bs); } // !SMT!-S
    int getSlots() const { return Util::toInt(get("SL")); } // !SMT!-S
	int getLimit() const { return Util::toInt(get("LI")); } // !SMT!-S

private:    
	inline bool getBIT(uint32_t p_mask) const 
	{
	 // Lock l(cs);
      return (m_info_bitmap & p_mask) > 0;	
	}
	inline void setBIT(uint32_t p_mask, bool p_f)
	{
 	 //	 Lock l(cs);
		 if(p_f) 
			 m_info_bitmap |= p_mask; 
		 else
			 m_info_bitmap &= ~p_mask;
	}
public:
#define GSBIT(x)\
	    bool is##x() const  { return getBIT(e_##x); }\
	    void set##x(bool p_f) { setBIT(e_##x, p_f);}

	enum {
		e_Op = 0x0001, // 'OP' - зовется чаще всех
		e_Hub = 0x0002, // 
		e_Hidden = 0x0004, // 
		e_Bot = 0x0008, // 
		e_Registered = 0x0010, //
		e_Away = 0x0020,  //
		e_BF = 0x0040, //
		e_TC = 0x0080, //
		e_FC = 0x0100, //
		e_BC = 0x0200 //
		//....
	};

	GSBIT(Op)
	GSBIT(Hub)
	GSBIT(Bot)
	GSBIT(Hidden)
	GSBIT(Registered)
	GSBIT(Away)
	GSBIT(BF)
	GSBIT(TC)
	GSBIT(FC)
	GSBIT(BC)
	uint32_t getFlags() const {
		return m_info_bitmap;
	}
//-----------------------------------------------
	const string getTag() const;
	bool supports(const string& name) const;
	bool isTcpActive() const { return (!user->isSet(User::NMDC) && !getIp().empty()) || !user->isSet(User::PASSIVE); }
	bool isUdpActive() const { return !getIp().empty() && !getUdpPort().empty(); }
	string getSIDString() const { uint32_t sid = getSID(); return string((const char*)&sid, 4); }
	
	void sendRawCommand(Client& c, const int aRawCommand) const;
	const string setCheat(Client& c, const string& aCheatDescription, bool aBadClient);
	uint32_t getSID() const { return Util::toUInt32(get("SI")); }
	void setSID(uint32_t sid) { if(sid != 0) set("SI", Util::toString(sid)); }
	const string getReport() const;
	const string updateClientType(OnlineUser& ou);
	bool matchProfile(const string& aString, const string& aProfile) const;

	void getParams(StringMap& map, const string& prefix, bool compatibility) const;
	UserPtr& getUser() { return user; }
    GETSET(UserPtr, user, User);

	const string get(const char* name) const;
	void set(const char* name, const string& val);

private:

	typedef HASH_MAP<short, string> InfMap;
	InfMap m_info_map;
	uint32_t m_info_bitmap; // битовая карта флажков (с) http://greylink.narod.ru/techinfo.html

	/** @todo there are probably more threading issues here ...*/
	mutable CriticalSection cs;
	string getVersion(const string& aExp, const string& aTag) const;
	string splitVersion(const string& aExp, const string& aTag, const int part) const;
};

class NmdcHub;

class OnlineUser : public FastAlloc<OnlineUser> {
public:

	OnlineUser(const UserPtr& ptr, Client& client_, uint32_t sid_);

	operator UserPtr&() { return getUser(); }
	operator const UserPtr&() const { return getUser(); }

	inline UserPtr& getUser() { return getIdentity().getUser(); }
	inline const UserPtr& getUser() const { return getIdentity().getUser(); }
	inline Identity& getIdentity() { return identity; }
	Client& getClient() { return client; }
	const Client& getClient() const { return client; }

	GETSET(Identity, identity, Identity);
private:
	friend class NmdcHub;

	OnlineUser(const OnlineUser&);
	OnlineUser& operator=(const OnlineUser&);

	Client& client;
};

#endif // !defined(USER_H)

/**
 * @file
 * $Id: User.h,v 1.3.2.1 2008/12/30 02:03:52 alexey Exp $
 */
