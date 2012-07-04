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

#ifndef DCPLUSPLUS_CLIENT_FAVORITE_MANAGER_H
#define DCPLUSPLUS_CLIENT_FAVORITE_MANAGER_H


#include "SettingsManager.h"

#include "CriticalSection.h"
//[-]PPA #include "HttpConnection.h"
#include "User.h"
#include "UserCommand.h"
#include "FavoriteUser.h"
#include "Singleton.h"
#include "ClientManagerListener.h"
#include "ClientManager.h"

class HubEntry {
public:
	typedef vector<HubEntry> List;
	typedef List::const_iterator Iter;
	
	HubEntry(const string& aName, const string& aServer, const string& aDescription, const string& aUsers) throw() : 
	name(aName), server(aServer), description(aDescription), country(Util::emptyString), 
	rating(Util::emptyString), reliability(0.0), shared(0), minShare(0), users(Util::toInt(aUsers)), minSlots(0), maxHubs(0), maxUsers(0) { }

	HubEntry(const string& aName, const string& aServer, const string& aDescription, const string& aUsers, const string& aCountry,
		const string& aShared, const string& aMinShare, const string& aMinSlots, const string& aMaxHubs, const string& aMaxUsers,
		const string& aReliability, const string& aRating) : name(aName), server(aServer), description(aDescription), country(aCountry), 
		rating(aRating), reliability((float)(Util::toDouble(aReliability) / 100.0)), shared(Util::toInt64(aShared)), minShare(Util::toInt64(aMinShare)),
		users(Util::toInt(aUsers)), minSlots(Util::toInt(aMinSlots)), maxHubs(Util::toInt(aMaxHubs)), maxUsers(Util::toInt(aMaxUsers)) 
	{

	}

	HubEntry() throw() { }
	HubEntry(const HubEntry& rhs) throw() : name(rhs.name), server(rhs.server), description(rhs.description), country(rhs.country), 
		rating(rhs.rating), reliability(rhs.reliability), shared(rhs.shared), minShare(rhs.minShare), users(rhs.users), minSlots(rhs.minSlots),
		maxHubs(rhs.maxHubs), maxUsers(rhs.maxUsers) { }

	~HubEntry() throw() { }

	GETSET(string, name, Name);
	GETSET(string, server, Server);
	GETSET(string, description, Description);
	GETSET(string, country, Country);
	GETSET(string, rating, Rating);
	GETSET(float, reliability, Reliability);
	GETSET(int64_t, shared, Shared);
	GETSET(int64_t, minShare, MinShare);
	GETSET(int, users, Users);
	GETSET(int, minSlots, MinSlots);
	GETSET(int, maxHubs, MaxHubs)
	GETSET(int, maxUsers, MaxUsers);
};

const string DEF_FAKE_ID = "FakeDC V:1.0"; // !SMT!-S

class FavoriteHubEntry {
public:
	typedef FavoriteHubEntry* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;

	FavoriteHubEntry() throw() : connect(false), windowposx(0), windowposy(0), windowsizex(0), 
                windowsizey(0), windowtype(0), chatusersplit(0), stealth(false), userliststate(true), hideShare(false), showJoins(false), exclChecks(false), mode(0), ip(Util::emptyString), overrideId(0), clientId(DEF_FAKE_ID) { } // !SMT!-S
	FavoriteHubEntry(const HubEntry& rhs) throw() : name(rhs.getName()), server(rhs.getServer()), 
		description(rhs.getDescription()), connect(false), windowposx(0), windowposy(0), windowsizex(0), 
                windowsizey(0), windowtype(0), chatusersplit(0), stealth(false), userliststate(true), hideShare(false), showJoins(false), exclChecks(false), mode(0), ip(Util::emptyString), overrideId(0), clientId(DEF_FAKE_ID)  { } // !SMT!-S
	FavoriteHubEntry(const FavoriteHubEntry& rhs) throw() : userdescription(rhs.userdescription), email(rhs.email), awaymsg(rhs.awaymsg),
		name(rhs.getName()), server(rhs.getServer()), description(rhs.getDescription()), password(rhs.getPassword()), connect(rhs.getConnect()), 
		nick(rhs.nick), windowposx(rhs.windowposx), windowposy(rhs.windowposy), windowsizex(rhs.windowsizex), 
		windowsizey(rhs.windowsizey), windowtype(rhs.windowtype), chatusersplit(rhs.chatusersplit), stealth(rhs.stealth),
		userliststate(rhs.userliststate), hideShare(rhs.hideShare), showJoins(rhs.showJoins), exclChecks(rhs.exclChecks), mode(rhs.mode), ip(rhs.ip),
                opChat(rhs.opChat), rawOne(rhs.rawOne), rawTwo(rhs.rawTwo), rawThree(rhs.rawThree), rawFour(rhs.rawFour), rawFive(rhs.rawFive), overrideId(rhs.overrideId), clientId(rhs.clientId) { }
	~FavoriteHubEntry() throw() { }
	
	const string& getNick(bool useDefault = true) const { 
		return (!nick.empty() || !useDefault) ? nick : SETTING(NICK);
	}

	void setNick(const string& aNick) { nick = aNick; }

	GETSET(string, userdescription, UserDescription);
	GETSET(string, awaymsg, AwayMsg);
	GETSET(string, email, Email);
	GETSET(string, name, Name);
	GETSET(string, server, Server);
	GETSET(string, description, Description);
	GETSET(string, password, Password);
	GETSET(string, headerOrder, HeaderOrder);
	GETSET(string, headerWidths, HeaderWidths);
	GETSET(string, headerVisible, HeaderVisible);
	GETSET(bool, connect, Connect);
	GETSET(int, windowposx, WindowPosX);
	GETSET(int, windowposy, WindowPosY);
	GETSET(int, windowsizex, WindowSizeX);
	GETSET(int, windowsizey, WindowSizeY);
	GETSET(int, windowtype, WindowType);
	GETSET(int, chatusersplit, ChatUserSplit);
	GETSET(bool, stealth, Stealth);
	GETSET(bool, userliststate, UserListState);
	GETSET(bool, hideShare, HideShare); // Hide Share Mod
	GETSET(bool, showJoins, ShowJoins); // Show joins
	GETSET(bool, exclChecks, ExclChecks); // Excl. from client checking
	GETSET(string, rawOne, RawOne);
	GETSET(string, rawTwo, RawTwo);
	GETSET(string, rawThree, RawThree);
	GETSET(string, rawFour, RawFour);
	GETSET(string, rawFive, RawFive);
	GETSET(int, mode, Mode); // 0 = default, 1 = active, 2 = passive
	GETSET(string, ip, IP);
	GETSET(string, opChat, OpChat);
        GETSET(string, clientId, ClientId); // !SMT!-S
        GETSET(bool, overrideId, OverrideId); // !SMT!-S

private:
	string nick;
};

class RecentHubEntry {
public:
	typedef RecentHubEntry* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;

	~RecentHubEntry() throw() { }	
	
	GETSET(string, name, Name);
	GETSET(string, server, Server);
	GETSET(string, description, Description);
	GETSET(string, users, Users);
	GETSET(string, shared, Shared);	
};

class PreviewApplication {
public:
	typedef PreviewApplication* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;

	PreviewApplication() throw() {}
	PreviewApplication(string n, string a, string r, string e) : name(n), application(a), arguments(r), extension(e) {};
	~PreviewApplication() throw() { }	

	GETSET(string, name, Name);
	GETSET(string, application, Application);
	GETSET(string, arguments, Arguments);
	GETSET(string, extension, Extension);
};

class FavoriteManagerListener {
public:
	virtual ~FavoriteManagerListener() { }
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> DownloadStarting;
	typedef X<1> DownloadFailed;
	typedef X<2> DownloadFinished;
	typedef X<3> FavoriteAdded;
	typedef X<4> FavoriteRemoved;
	typedef X<5> UserAdded;
	typedef X<6> UserRemoved;
	typedef X<7> StatusChanged;
	typedef X<8> RecentAdded;
	typedef X<9> RecentRemoved;
	typedef X<10> RecentUpdated;

	virtual void on(DownloadStarting, const string&) throw() { }
	virtual void on(DownloadFailed, const string&) throw() { }
	virtual void on(DownloadFinished, const string&) throw() { }
	virtual void on(FavoriteAdded, const FavoriteHubEntry*) throw() { }
	virtual void on(FavoriteRemoved, const FavoriteHubEntry*) throw() { }
	virtual void on(UserAdded, const FavoriteUser&) throw() { }
	virtual void on(UserRemoved, const FavoriteUser&) throw() { }
	virtual void on(StatusChanged, const UserPtr&) throw() { }
	virtual void on(RecentAdded, const RecentHubEntry*) throw() { }
	virtual void on(RecentRemoved, const RecentHubEntry*) throw() { }
	virtual void on(RecentUpdated, const RecentHubEntry*) throw() { }
};

class FavoriteManagerInitializer {
public:
  virtual void onLoad(int& version, FavoriteHubEntry::List& /*favoriteHubs*/) const = 0;
};

class SimpleXML;

/**
 * Public hub list, favorites (hub&user). Assumed to be called only by UI thread.
 */
class FavoriteManager : public Speaker<FavoriteManagerListener>
//[-]PPA	, private HttpConnectionListener
	, public Singleton<FavoriteManager>,
	private SettingsManagerListener, private ClientManagerListener
{
public:
// Public Hubs
//[-]PPA	enum HubTypes {
//[-]PPA		TYPE_NORMAL,
//[-]PPA		TYPE_BZIP2
//[-]PPA	};
//[-]PPA	StringList getHubLists();
//[-]PPA	bool setHubList(int /*aHubList*/);
//[-]PPA	unsigned int getSelectedHubList() { return lastServer; }
//[-]PPA	void refresh();
//[-]PPA	HubTypes getHubListType() { return listType; }
//[-]PPA 	HubEntry::List getPublicHubs() {
//[-]PPA 		Lock l(cs);
//[-]PPA 		return publicListMatrix[publicListServer];
//[-]PPA 	}
//[-]PPA 	bool isDownloading() { return running; }

// Favorite Users
	typedef HASH_MAP_X(CID, FavoriteUser, CID::Hash, equal_to<CID>, less<CID>) FavoriteMap;
	FavoriteMap getFavoriteUsers() { Lock l(cs); return users; }
	PreviewApplication::List& getPreviewApps() { return previewApplications; }

	void addFavoriteUser(UserPtr& aUser);
	bool isFavoriteUser(const UserPtr& aUser) const { Lock l(cs); return users.find(aUser->getCID()) != users.end(); }
	void removeFavoriteUser(UserPtr& aUser);

	bool hasSlot(const UserPtr& aUser) const;

        // !SMT!-S
        bool hasBan(const UserPtr& aUser) const;
        void setUploadLimit(const UserPtr& aUser, FavoriteUser::UPLOAD_LIMIT lim);
        bool getFlag(const UserPtr& aUser, FavoriteUser::Flags) const;
        void setFlag(const UserPtr& aUser, FavoriteUser::Flags, bool);
        bool hasIgnore(const UserPtr& aUser) const { return getFlag(aUser, FavoriteUser::FLAG_IGNOREPRIVATE); }
        void setIgnorePrivate(const UserPtr& aUser, bool grant) { setFlag(aUser, FavoriteUser::FLAG_IGNOREPRIVATE, grant); }
        bool hasFreePM(const UserPtr& aUser) const { return getFlag(aUser, FavoriteUser::FLAG_FREE_PM_ACCESS); }
        void setFreePM(const UserPtr& aUser, bool grant) { setFlag(aUser, FavoriteUser::FLAG_FREE_PM_ACCESS, grant); }

	void setUserDescription(const UserPtr& aUser, const string& description);
	void setAutoGrant(const UserPtr& aUser, bool grant);
        void setSuperUser(const UserPtr& aUser, bool superUser);

        void userUpdated(const UserPtr& info); // !SMT!-fix
	time_t getLastSeen(const UserPtr& aUser) const;
// Favorite Hubs
	FavoriteHubEntry::List& getFavoriteHubs() { return favoriteHubs; }

	void addFavorite(const FavoriteHubEntry& aEntry);
	void removeFavorite(FavoriteHubEntry* entry);
	bool checkFavHubExists(const FavoriteHubEntry& aEntry);
	FavoriteHubEntry* getFavoriteHubEntry(const string& aServer);

// Favorite Directories
	bool addFavoriteDir(const string& aDirectory, const string& aName);
	bool removeFavoriteDir(const string& aName);
	bool renameFavoriteDir(const string& aName, const string& anotherName);
	StringPairList getFavoriteDirs() { return favoriteDirs; }

// Recent Hubs
	RecentHubEntry::List& getRecentHubs() { return recentHubs; };

	void addRecent(const RecentHubEntry& aEntry);
	void removeRecent(const RecentHubEntry* entry);
	void updateRecent(const RecentHubEntry* entry);

	RecentHubEntry* getRecentHubEntry(const string& aServer) {
		for(RecentHubEntry::Iter i = recentHubs.begin(); i != recentHubs.end(); ++i) {
			RecentHubEntry* r = *i;
			if(Util::stricmp(r->getServer(), aServer) == 0) {
				return r;
			}
		}
		return NULL;
	}

	PreviewApplication* addPreviewApp(string name, string application, string arguments, string extension){
		PreviewApplication* pa = new PreviewApplication(name, application, arguments, extension);
		previewApplications.push_back(pa);
		return pa;
	}

	PreviewApplication* removePreviewApp(unsigned int index){
		if(previewApplications.size() > index)
			previewApplications.erase(previewApplications.begin() + index);	
		return NULL;
	}

	PreviewApplication* getPreviewApp(unsigned int index, PreviewApplication &pa){
		if(previewApplications.size() > index)
			pa = *previewApplications[index];	
		return NULL;
	}
	
	PreviewApplication* updatePreviewApp(int index, PreviewApplication &pa){
		*previewApplications[index] = pa;
		return NULL;
	}

	void removeallRecent() {
		recentHubs.clear();
		recentsave();
	}

// User Commands
	UserCommand addUserCommand(int type, int ctx, Flags::MaskType flags, const string& name, const string& command, const string& hub);
	bool getUserCommand(int cid, UserCommand& uc);
	int findUserCommand(const string& aName);
	bool moveUserCommand(int cid, int pos);
	void updateUserCommand(const UserCommand& uc);
	void removeUserCommand(int cid);
	void removeUserCommand(const string& srv);
	void removeHubUserCommands(int ctx, const string& hub);

	UserCommand::List getUserCommands() { Lock l(cs); return userCommands; }
	UserCommand::List getUserCommands(int ctx, const StringList& hub, bool& op);

	void load(const FavoriteManagerInitializer* initializer);
	void save();
	void recentsave();
	
private:
	int m_version;
	FavoriteHubEntry::List favoriteHubs;
	StringPairList favoriteDirs;
	RecentHubEntry::List recentHubs;
	PreviewApplication::List previewApplications;
	UserCommand::List userCommands;
	int lastId;

	FavoriteMap users;

	mutable CriticalSection cs;

	// Public Hubs
//[-]PPA 	typedef map<string, HubEntry::List> PubListMap;
//[-]PPA 	PubListMap publicListMatrix;
//[-]PPA 	string publicListServer;
//[-]PPA 	bool running;
//[-]PPA 	HttpConnection* c;
//[-]PPA	unsigned int lastServer;
//[-]PPA	HubTypes listType;
//[-]PPA 	string downloadBuf;
	
	/** Used during loading to prevent saving. */
	bool dontSave;

	friend class Singleton<FavoriteManager>;
	
	FavoriteManager() : lastId(0), 
          m_version(0),
//[-]PPA 		running(false), c(NULL), 
//[-]PPA		lastServer(0), 
//[-]PPA		listType(TYPE_NORMAL), 
		dontSave(false) {
		SettingsManager::getInstance()->addListener(this);
		ClientManager::getInstance()->addListener(this);
	}

	virtual ~FavoriteManager() throw() {
		ClientManager::getInstance()->removeListener(this);
		SettingsManager::getInstance()->removeListener(this);
//[-]PPA 		if(c) {
//[-]PPA 			c->removeListener(this);
//[-]PPA 			delete c;
//[-]PPA 			c = NULL;
//[-]PPA 		}
		
		for_each(favoriteHubs.begin(), favoriteHubs.end(), DeleteFunction());
		for_each(recentHubs.begin(), recentHubs.end(), DeleteFunction());
		for_each(previewApplications.begin(), previewApplications.end(), DeleteFunction());
	}
	
	FavoriteHubEntry::Iter getFavoriteHub(const string& aServer) {
		for(FavoriteHubEntry::Iter i = favoriteHubs.begin(); i != favoriteHubs.end(); ++i) {
			if(Util::stricmp((*i)->getServer(), aServer) == 0) {
				return i;
			}
		}
		return favoriteHubs.end();
	}

//[-]PPA	void loadXmlList(const string& xml);

	RecentHubEntry::Iter getRecentHub(const string& aServer) const {
		for(RecentHubEntry::Iter i = recentHubs.begin(); i != recentHubs.end(); ++i) {
			if(Util::stricmp((*i)->getServer(), aServer) == 0) {
				return i;
			}
		}
		return recentHubs.end();
	}

	// ClientManagerListener
        virtual void on(UserUpdated, const UserPtr& user) throw();
	virtual void on(UserConnected, const UserPtr& user) throw();
	virtual void on(UserDisconnected, const UserPtr& user) throw();

	// HttpConnectionListener
//[-]PPA 	virtual void on(Data, HttpConnection*, const uint8_t*, size_t) throw();
//[-]PPA 	virtual void on(Failed, HttpConnection*, const string&) throw();
//[-]PPA 	virtual void on(Complete, HttpConnection*, const string&) throw();
//[-]PPA 	virtual void on(Redirected, HttpConnection*, const string&) throw();
//[-]PPA 	virtual void on(TypeNormal, HttpConnection*) throw();
//[-]PPA 	virtual void on(TypeBZ2, HttpConnection*) throw();

//[-]PPA 	void onHttpFinished() throw();

	// SettingsManagerListener
	virtual void on(SettingsManagerListener::Load, SimpleXML& xml) throw() {
		previewload(xml);
	}

	virtual void on(SettingsManagerListener::Save, SimpleXML& xml) throw() {
		previewsave(xml);
	}

	void load(SimpleXML& aXml);
	void recentload(SimpleXML& aXml);
	void previewload(SimpleXML& aXml);
	void previewsave(SimpleXML& aXml);
	
	string getConfigFile() { return Util::getConfigPath() + "Favorites.xml"; }
};

#endif // !defined(FAVORITE_MANAGER_H)

/**
 * @file
 * $Id: FavoriteManager.h,v 1.3 2008/03/31 15:46:07 alexey Exp $
 */
