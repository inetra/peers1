#if !defined(__CLIENT_PROFILE_MANAGER_H)
#define __CLIENT_PROFILE_MANAGER_H


#include "Util.h"
#include "CriticalSection.h"
#include "Singleton.h"

class ClientProfile {
	public:
	typedef vector<ClientProfile> List;

	ClientProfile() : id(0), priority(0), rawToSend(0) { };

	ClientProfile(int aId, const string& aName, const string& aVersion, const string& aTag, 
		const string& aExtendedTag, const string& aLock, const string& aPk, const string& aSupports,
		const string& aTestSUR, const string& aUserConCom, const string& aStatus, const string& aCheatingDescription, 
		int aRawToSend, /*int aTagVersion,*/ int aUseExtraVersion, int aCheckMismatch,
		const string& aConnection, const string& aComment, int aRecheck, int aSkipExtended) 
		throw() : id(aId), name(aName), version(aVersion), tag(aTag), extendedTag(aExtendedTag), 
		lock(aLock), pk(aPk), supports(aSupports), testSUR(aTestSUR), userConCom(aUserConCom), status(aStatus),
		cheatingDescription(aCheatingDescription), rawToSend(aRawToSend), /*tagVersion(aTagVersion), */
		useExtraVersion(aUseExtraVersion), checkMismatch(aCheckMismatch), connection(aConnection), comment(aComment),
		recheck(aRecheck), skipExtended(aSkipExtended)
	{
	
	};

	ClientProfile(const ClientProfile& rhs) : id(rhs.id), name(rhs.name), version(rhs.version), tag(rhs.tag), 
		extendedTag(rhs.extendedTag), lock(rhs.lock), pk(rhs.pk), supports(rhs.supports), testSUR(rhs.testSUR), 
		userConCom(rhs.userConCom), status(rhs.status), cheatingDescription(rhs.cheatingDescription), rawToSend(rhs.rawToSend), 
		/*tagVersion(rhs.tagVersion),*/ useExtraVersion(rhs.useExtraVersion), checkMismatch(rhs.checkMismatch),
		connection(rhs.connection), comment(rhs.comment), recheck(rhs.recheck), skipExtended(rhs.skipExtended)
	{

	}

	ClientProfile& operator=(const ClientProfile& rhs) { 
		id = rhs.id;
		name = rhs.name;
		version = rhs.version;
		tag = rhs.tag;
		extendedTag = rhs.extendedTag;
		lock = rhs.lock;
		pk = rhs.pk;
		supports = rhs.supports;
		testSUR = rhs.testSUR;
		userConCom = rhs.userConCom;
		status = rhs.status;
		rawToSend = rhs.rawToSend;
		cheatingDescription = rhs.cheatingDescription;
		//tagVersion = rhs.tagVersion;
		useExtraVersion = rhs.useExtraVersion;
		checkMismatch = rhs.checkMismatch;
		connection = rhs.connection;
		comment = rhs.comment;
		recheck = rhs.recheck;
		skipExtended = rhs.skipExtended;
		return *this;
	}

		GETSET(int, id, Id);
		GETSET(string, name, Name);
		GETSET(string, version, Version);
		GETSET(string, tag, Tag);
		GETSET(string, extendedTag, ExtendedTag);
		GETSET(string, lock, Lock);
		GETSET(string, pk, Pk);
		GETSET(string, supports, Supports);
		GETSET(string, testSUR, TestSUR);
		GETSET(string, userConCom, UserConCom);
		GETSET(string, status, Status);
		GETSET(string, cheatingDescription, CheatingDescription);
		GETSET(string, connection, Connection);
		GETSET(string, comment, Comment);
		GETSET(int, priority, Priority);
		GETSET(int, rawToSend, RawToSend);
		//GETSET(int, tagVersion, TagVersion);
		GETSET(int, useExtraVersion, UseExtraVersion);
		GETSET(int, checkMismatch, CheckMismatch);
		GETSET(int, recheck, Recheck);
		GETSET(int, skipExtended, SkipExtended);
};


class SimpleXML;

class ClientProfileManager : public Singleton<ClientProfileManager>
{
public:
	StringMap& getParams() {
		Lock l(ccs);
		return params;
	}	
	ClientProfile::List& getClientProfiles() {
		Lock l(ccs);
		return clientProfiles;
	}

	ClientProfile::List& getClientProfiles(StringMap &paramList) {
		Lock l(ccs);
		paramList = params;
		return clientProfiles;
	}
	
	ClientProfile addClientProfile(
		const string& name, 
		const string& version, 
		const string& tag, 
		const string& extendedTag, 
		const string& lock, 
		const string& pk, 
		const string& supports, 
		const string& testSUR, 
		const string& userConCom, 
		const string& status,
		const string& cheatingdescription, 
		int rawToSend,
		//int tagVersion, 
		int useExtraVersion, 
		int checkMismatch,
		const string& connection,
		const string& comment,
		int recheck,
		int skipExtended
		) 
	{
		Lock l(ccs);
		clientProfiles.push_back(
			ClientProfile(
			lastProfile++, 
			name, 
			version, 
			tag, 
			extendedTag, 
			lock, 
			pk, 
			supports, 
			testSUR, 
			userConCom, 
			status, 
			cheatingdescription,
			rawToSend, 
			//tagVersion, 
			useExtraVersion, 
			checkMismatch,
			connection,
			comment,
			recheck,
			skipExtended
			)
		);
		return clientProfiles.back();
	}

	void addClientProfile( const StringList& sl ) {
		Lock l(ccs);
		clientProfiles.push_back(
			ClientProfile(
			lastProfile++, 
			sl[0], 
			sl[1], 
			sl[2], 
			sl[3], 
			sl[4], 
			sl[5], 
			sl[6], 
			sl[7], 
			sl[8], 
			sl[9], 
			"", 
			0, 
			//0, 
			0, 
			0
			// FIXME
			, "", "", 0, 0
			)
		);
//		save();
	}

	bool getClientProfile(int id, ClientProfile& cp) {
		Lock l(ccs);
		for(ClientProfile::List::const_iterator i = clientProfiles.begin(); i != clientProfiles.end(); ++i) {
			if(i->getId() == id) {
				cp = *i;
				return true;
			}
		}
		return false;
	}

	void removeClientProfile(int id) {
		Lock l(ccs);
		for(ClientProfile::List::iterator i = clientProfiles.begin(); i != clientProfiles.end(); ++i) {
			if(i->getId() == id) {
				clientProfiles.erase(i);
				break;
			}
		}
	}

	void updateClientProfile(const ClientProfile& cp) {
		Lock l(ccs);
		for(ClientProfile::List::iterator i = clientProfiles.begin(); i != clientProfiles.end(); ++i) {
			if(i->getId() == cp.getId()) {
				*i = cp;
				break;
			}
		}
	}

	bool moveClientProfile(int id, int pos) {
		dcassert(pos == -1 || pos == 1);
		Lock l(ccs);
		for(ClientProfile::List::iterator i = clientProfiles.begin(); i != clientProfiles.end(); ++i) {
			if(i->getId() == id) {
				swap(*i, *(i + pos));
				return true;
			}
		}
		return false;
	}

	ClientProfile::List& reloadClientProfiles() { 
		Lock l(ccs);
		clientProfiles.clear();
		params.clear();
		loadClientProfiles();
		return clientProfiles;
	}
	ClientProfile::List& reloadClientProfilesFromHttp() { 
		Lock l(ccs);
		ClientProfile::List oldProfiles = clientProfiles;
		clientProfiles.clear();
		params.clear();
		loadClientProfiles();
		for(ClientProfile::List::iterator j = clientProfiles.begin(); j != clientProfiles.end(); ++j) {
			for(ClientProfile::List::const_iterator k = oldProfiles.begin(); k != oldProfiles.end(); ++k) {
				if((*k).getName().compare((*j).getName()) == 0) {
					(*j).setRawToSend((*k).getRawToSend());
					(*j).setCheatingDescription((*k).getCheatingDescription());
				}
			}
		}
		return clientProfiles;
	}


	void load() {
		loadClientProfiles();
	};
	//void save() {};


	void loadClientProfiles();

	void saveClientProfiles();
private:
	ClientProfile::List clientProfiles;
	int lastProfile;
	StringMap params;

	CriticalSection ccs;

	friend class Singleton<ClientProfileManager>;
	
	ClientProfileManager() : lastProfile(0) {}

	~ClientProfileManager() {
	}
	
	void loadClientProfiles(SimpleXML* aXml);
};

#endif // !defined(__CLIENT_PROFILE_MANAGER_H)
