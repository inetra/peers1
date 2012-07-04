///////////////////////////////////////////////////////////////////////////////
//
//	Handles saving and loading of ignorelists
//
///////////////////////////////////////////////////////////////////////////////

#include "stdinc.h"
#include "IgnoreManager.h"

#include "Util.h"
#include "pme.h"
#include "Wildcards.h"

void IgnoreManager::load(SimpleXML& aXml) {
	if(aXml.findChild("IgnoreList")) {
		aXml.stepIn();
		while(aXml.findChild("User")) {	
			ignoredUsers.insert(aXml.getChildAttrib("Nick"));
		}
		aXml.stepOut();
	}
}

void IgnoreManager::save(SimpleXML& aXml) {
	aXml.addTag("IgnoreList");
	aXml.stepIn();

	for(StringHash::const_iterator i = ignoredUsers.begin(); i != ignoredUsers.end(); ++i) {
		aXml.addTag("User");
		aXml.addChildAttrib("Nick", *i);
	}
	aXml.stepOut();
}

void IgnoreManager::storeIgnore(const UserPtr& user) {
	Lock l(cs);
	ignoredUsers.insert(user->getFirstNick());
}

void IgnoreManager::removeIgnore(const UserPtr& user) {
	Lock l(cs);
	ignoredUsers.erase(user->getFirstNick());
}

bool IgnoreManager::isIgnored(const string& aNick) {
	Lock l(cs);
	if(ignoredUsers.find(aNick) != ignoredUsers.end()) {
		return true;
	}
	if (BOOLSETTING(IGNORE_USE_REGEXP_OR_WC)) {
		for (StringHash::const_iterator i = ignoredUsers.begin(); i != ignoredUsers.end(); ++i) {
			const string tmp = *i;
			if (Util::strnicmp(tmp, "$Re:", 4) == 0) {
				if (tmp.length() > 4) {
					PME regexp(tmp.substr(4), "gims");
					if (regexp.match(aNick)) {
						return true;
					}
				}
			} 
			else {
				if (Wildcard::patternMatch(Text::toLower(aNick), Text::toLower(tmp), false)) {
					return true;
				}
			}
		}
	}
	return false;
}

// SettingsManagerListener
void IgnoreManager::on(SettingsManagerListener::Load, SimpleXML& aXml) {
	load(aXml);
}

void IgnoreManager::on(SettingsManagerListener::Save, SimpleXML& aXml) {
	save(aXml);
}

void IgnoreManager::getIgnoredUsers(WStringList& users) { 
	Lock l(cs); 
	for (StringHash::iterator i = ignoredUsers.begin(); i != ignoredUsers.end(); ++i) {
		users.push_back(Text::toT(*i));
	}
}
