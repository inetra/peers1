///////////////////////////////////////////////////////////////////////////////
//
//	Handles saving and loading of ignorelists
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IGNOREMANAGER_H
#define IGNOREMANAGER_H

#include "DCPlusPlus.h"

#include "Singleton.h"
#include "SettingsManager.h"
#include "SimpleXML.h"
#include "User.h"

class IgnoreManager: public Singleton<IgnoreManager>, private SettingsManagerListener
{
public:
	IgnoreManager() { SettingsManager::getInstance()->addListener(this); }
	~IgnoreManager() { SettingsManager::getInstance()->removeListener(this); }

	// store & remove ignores through/from hubframe
	void storeIgnore(const UserPtr& user);
	void removeIgnore(const UserPtr& user);

	// check if user is ignored
	bool isIgnored(const string& aNick);
	bool isIgnored(const UserPtr& user) { return isIgnored(user->getFirstNick()); }

	// get and put ignorelist (for MiscPage)
	hash_set<string> getIgnoredUsers() { Lock l(cs); return ignoredUsers; }
	void getIgnoredUsers(WStringList& users);
	void putIgnoredUsers(hash_set<string> ignoreList) { Lock l(cs); ignoredUsers = ignoreList; }

private:
	typedef hash_set<string> StringHash;
	CriticalSection cs;

	// save & load
	void load(SimpleXML& aXml);
	void save(SimpleXML& aXml);

	// SettingsManagerListener
	virtual void on(SettingsManagerListener::Load, SimpleXML& xml) throw();
	virtual void on(SettingsManagerListener::Save, SimpleXML& xml) throw();

	// contains the ignored nicks and patterns 
	StringHash ignoredUsers;
};

#endif // IGNOREMANAGER_H