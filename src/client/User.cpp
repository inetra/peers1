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

#include "stdinc.h"
#include "DCPlusPlus.h"

//[-]PPA [Doxygen 1.5.1] #include "User.h"
//[-]PPA [Doxygen 1.5.1] #include "Client.h"
#include "StringTokenizer.h"
#include "FavoriteUser.h"

#include "ClientManager.h"
#include "ClientProfileManager.h"
#include "pme.h"
#include "UserCommand.h"
#include "ResourceManager.h"
#include "ConnectionManager.h" //[+]PPA
#include "../peers/Sounds.h"

OnlineUser::OnlineUser(const UserPtr& ptr, Client& client_, uint32_t sid_) : identity(ptr, sid_), client(client_) { 

}

void Identity::getParams(StringMap& sm, const string& prefix, bool compatibility) const {
	{
		Lock l(cs);
		for(InfMap::const_iterator i = m_info_map.begin(); i != m_info_map.end(); ++i) {
			sm[prefix + string((char*)(&i->first), 2)] = i->second;
		}
	}
	if(user) {
		const string l_CIDBase32 = user->getCID().toBase32(); //[+]PPA
		sm[prefix + "SID"] = getSIDString();
		sm[prefix + "CID"] = l_CIDBase32;
		sm[prefix + "TAG"] = getTag();
		const string l_SS_orig = getSS();
		const string l_SS = Util::formatBytes(l_SS_orig);
		sm[prefix + "SSshort"] = l_SS;
		if(compatibility) {
			if(prefix == "my") {
				sm["mynick"] = getNick();
				sm["mycid"] = l_CIDBase32; //[+]PPA
			} else {
				sm["nick"] = getNick();
				sm["cid"] = l_CIDBase32; //[+]PPA
				sm["ip"] = get("I4");
				sm["tag"] = getTag();
				sm["description"] = get("DE");
				sm["email"] = get("EM");
				sm["share"] = l_SS_orig;
				sm["shareshort"] = l_SS;
				sm["realshareformat"] = Util::formatBytes(get("RS"));
			}
		}
	}
}

const string Identity::getTag() const {
	const string l_TA = get("TA");
	if(!l_TA.empty())
		return l_TA;
	const string l_VE = get("VE");
	const string l_HN = get("HN");
	const string l_HR = get("HR");
	const string l_HO = get("HO");
	const string l_SL = get("SL");
	if(l_VE.empty() || l_HN.empty() || l_HR.empty() ||l_HO.empty() || !Util::toInt(l_SL))
		return Util::emptyString;
	return "<" + l_VE + ",M:" + string(isTcpActive() ? "A" : "P") + ",H:" + l_HN + "/" +
		l_HR + "/" + l_HO + ",S:" + l_SL + ">";
}
const string Identity::get(const char* name) const {
	Lock l(cs);
	InfMap::const_iterator i = m_info_map.find(*(short*)name);
#if 0
	static LokiMap<string, int> g_cnt;
	g_cnt[name]++;
	if(g_cnt[name] %100 == 0)
	{
		dcdebug("get[%s] = %d \n",name,g_cnt[name]);
	} 
    /*
	if(m_info_map.capacity() > 20)
	{
		dcdebug("capacity = %d m_info_map.size() = %d\n",m_info_map.capacity(),m_info_map.size());
	}
	*/
#endif	
	return i == m_info_map.end() ? Util::emptyString : i->second;
}

void Identity::set(const char* name, const string& val) {
	Lock l(cs);
#if 0
	static LokiMap<string, int> g_cnt;
	g_cnt[name]++;
	if(g_cnt[name] %100 == 0)
	{
		dcdebug("set[%s] = %d \n",name,g_cnt[name]);
	} 
#endif
	m_info_map[*(short*)name] = val;
}

bool Identity::supports(const string& name) const {
	const string su = get("SU");
	StringTokenizer<string> st(su, ',');
	for(StringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
		if(*i == name)
			return true;
	}
	return false;
}

string Identity::getConnection() const {
	const string connection = get("US");
	if(connection.find_first_not_of("0123456789.,") == string::npos) {
		double us = Util::toDouble(connection);
		if(us > 0) {
			char buf[16];
			snprintf(buf, sizeof(buf), "%.3g", us / 1024 / 1024);

			char *cp;
			if( (cp=strchr(buf, ',')) != NULL) *cp='.';

			return buf;
		} else {
			return connection;
		}
	} else {
		return connection;
	}
}

void FavoriteUser::update(const UserPtr& info) { // !SMT!-fix
        ClientManager::LockInstance l_instance; // !SMT!-fix
        if (OnlineUser *ou = ClientManager::getInstance()->getOnlineUser(info)) { // !SMT!-fix
                setNick(ou->getIdentity().getNick());
                setUrl(ou->getClient().getHubUrl());
        }
}

const string Identity::setCheat(Client& c, const string& aCheatDescription, bool aBadClient) {
	if(!c.isOp() || isOp()) return Util::emptyString;

	Sounds::PlaySound(SettingsManager::FAKERFILE);
		
	StringMap ucParams;
	getParams(ucParams, "user", true);
	string cheat = Util::formatParams(aCheatDescription, ucParams, false);
	
	set("CS", cheat);
	setBC(aBadClient);

	string report = "*** " + STRING(USER) + " " + getNick() + " - " + cheat;
	return report;
}

const string Identity::getReport() const {
	string report = "\r\nClient:		" + get("CT");
	report += "\r\nXML Generator:	" + (get("GE").empty() ? "N/A" : get("GE"));
	report += "\r\nLock:		" + get("LO");
	report += "\r\nPk:		" + get("PK");
	report += "\r\nTag:		" + getTag();
	report += "\r\nSupports:		" + get("SU");
	report += "\r\nStatus:		" + Util::formatStatus(Util::toInt(getStatus()));
	report += "\r\nTestSUR:		" + get("TS");
	report += "\r\nDisconnects:	" + get("FD");
	report += "\r\nTimeouts:		" + get("TO");
	report += "\r\nDownspeed:	" + Util::toString(getUser()->getLastDownloadSpeed()) + " kB/s";
	report += "\r\nIP:		" + getIp();
	report += "\r\nHost:		" + Socket::getRemoteHost(getIp());
	report += "\r\nDescription:	" + getDescription();
	report += "\r\nEmail:		" + getEmail();
	report += "\r\nConnection:	" + getConnection();
	report += "\r\nCommands:	" + get("UC");

	int64_t listSize =  (!get("LS").empty()) ? Util::toInt64(get("LS")) : -1;
	report += "\r\nFilelist size:	" + ((listSize != -1) ? (string)(Util::formatBytes(listSize) + "  (" + Text::fromT(Util::formatExactSize(listSize)) + " )") : "N/A");
	
	int64_t listLen = (!get("LL").empty()) ? Util::toInt64(get("LL")) : -1;
	report += "\r\nListLen:		" + ((listLen != -1) ? (string)(Util::formatBytes(listLen) + "  (" + Text::fromT(Util::formatExactSize(listLen)) + " )") : "N/A");
	report += "\r\nStated Share:	" + Util::formatBytes(getBytesShared()) + "  (" + Text::fromT(Util::formatExactSize(getBytesShared())) + " )";
	
	int64_t realBytes =  (!get("RS").empty()) ? Util::toInt64(get("RS")) : -1;
	report += "\r\nReal Share:	" + ((realBytes > -1) ? (string)(Util::formatBytes(realBytes) + "  (" + Text::fromT(Util::formatExactSize(realBytes)) + " )") : "N/A");
	report += "\r\nCheat status:	" + (get("CS").empty() ? "N/A" : get("CS"));
	report += "\r\nComment:		" + get("CM");
	return report;
}

const string Identity::updateClientType(OnlineUser& ou) {
	if ( getUser()->isSet(User::DCPLUSPLUS) && (get("LL") == "11") && (getBytesShared() > 0) ) {
		string report = setCheat(ou.getClient(), "Fake file list - ListLen = 11" , true);
		set("CT", "DC++ Stealth");
		setBC(true);
		setBF(true);
		sendRawCommand(ou.getClient(), SETTING(LISTLEN_MISMATCH));
		return report;
	} else if( getUser()->isSet(User::DCPLUSPLUS) &&
		strncmp(getTag().c_str(), "<++ V:0.69", 10) == 0 &&
		get("LL") != "42") {
			string report = setCheat(ou.getClient(), "Listlen mismatched" , true);
			set("CT", "Faked DC++");
			set("CM", "Supports corrupted files...");
			setBC(true);
			sendRawCommand(ou.getClient(), SETTING(LISTLEN_MISMATCH));
			return report;
	}
	int64_t tick = GET_TICK();

	StringMap params;
	ClientProfile::List& lst = ClientProfileManager::getInstance()->getClientProfiles(params);

	for(ClientProfile::List::const_iterator i = lst.begin(); i != lst.end(); ++i) {
		const ClientProfile& cp = *i;
		string version, pkVersion, extraVersion, formattedTagExp, verTagExp;

		verTagExp = Util::formatRegExp(cp.getTag(), params);

		formattedTagExp = verTagExp;
		string::size_type j = formattedTagExp.find("%[version]");
		if(j != string::npos) {
			formattedTagExp.replace(j, 10, ".*");
		}

		string pkExp = cp.getPk();
		string formattedPkExp = pkExp;
		j = pkExp.find("%[version]");
		if(j != string::npos) {
			formattedPkExp.replace(j, 10, ".*");
		}
		string extTagExp = cp.getExtendedTag();
		string formattedExtTagExp = extTagExp;
		j = extTagExp.find("%[version2]");
		if(j != string::npos) {
			formattedExtTagExp.replace(j, 11, ".*");
		}

		DETECTION_DEBUG("\tChecking profile: " + cp.getName());

		if (!matchProfile(get("LO"), cp.getLock())) { continue; }
		if (!matchProfile(getTag(), formattedTagExp)) { continue; } 
		if (!matchProfile(get("PK"), formattedPkExp)) { continue; }
		if (!matchProfile(get("SU"), cp.getSupports())) { continue; }
		if (!matchProfile(get("TS"), cp.getTestSUR())) { continue; }
		if (!matchProfile(getStatus(), cp.getStatus())) { continue; }
		if (!matchProfile(get("UC"), cp.getUserConCom())) { continue; }
		if (!matchProfile(getDescription(), formattedExtTagExp))	{ continue; }
		if (!matchProfile(getConnection(), cp.getConnection()))	{ continue; }

		if (verTagExp.find("%[version]") != string::npos) { version = getVersion(verTagExp, getTag()); }
		if (extTagExp.find("%[version2]") != string::npos) { extraVersion = getVersion(extTagExp, getDescription()); }
		if (pkExp.find("%[version]") != string::npos) { pkVersion = getVersion(pkExp, get("PK")); }

		if (!(cp.getVersion().empty()) && !matchProfile(version, cp.getVersion())) { continue; }

		DETECTION_DEBUG("Client found: " + cp.getName() + " time taken: " + Util::toString(GET_TICK()-tick) + " milliseconds");
		if (cp.getUseExtraVersion()) {
			set("CT", cp.getName() + " " + extraVersion );
		} else {
			set("CT", cp.getName() + " " + version);
		}
		set("CS", cp.getCheatingDescription());
		set("CM", cp.getComment());
		setBC(!cp.getCheatingDescription().empty());

		if (cp.getCheckMismatch() && version.compare(pkVersion) != 0) { 
			set("CT", get("CT") + " Version mis-match");
			set("CS", get("CS") + " Version mis-match");
			setBC(true);
			string report = setCheat(ou.getClient(), get("CS"), true);
			return report;
		}
		string report = Util::emptyString;
		if(isBC())
			report = setCheat(ou.getClient(), get("CS"), true);
		if(cp.getRawToSend() > 0) {
			sendRawCommand(ou.getClient(), cp.getRawToSend());
		}
		return report;
	}
	set("CT", "Unknown");
	set("CS", Util::emptyString);
	setBC(false);

	return Util::emptyString;
}

bool Identity::matchProfile(const string& aString, const string& aProfile) const {
	DETECTION_DEBUG("\t\tMatching String: " + aString + " to Profile: " + aProfile);
	PME reg(aProfile);
	return reg.IsValid() ? (reg.match(aString) > 0) : false;
}

string Identity::getVersion(const string& aExp, const string& aTag) const {
	string::size_type i = aExp.find("%[version]");
	if (i == string::npos) { 
		i = aExp.find("%[version2]"); 
		return splitVersion(aExp.substr(i + 11), splitVersion(aExp.substr(0, i), aTag, 1), 0);
	}
	return splitVersion(aExp.substr(i + 10), splitVersion(aExp.substr(0, i), aTag, 1), 0);
}

string Identity::splitVersion(const string& aExp, const string& aTag, const int part) const {
	PME reg(aExp);
	if(!reg.IsValid()) { return ""; }
	reg.split(aTag, 2);
	return reg[part];
}

void Identity::sendRawCommand(Client& c, const int aRawCommand) const {
	string rawCommand = c.getRawCommand(aRawCommand);
	if (!rawCommand.empty()) {
		StringMap ucParams;

		UserCommand uc = UserCommand(0, 0, 0, 0, "", rawCommand, "");
		ClientManager::getInstance()->userCommand(user, uc, ucParams, true);
	}
}
bool User::hasAutoBan() const
{
	return  !ConnectionManager::m_DisableAutoBan && //[+]PPA
		( getCountSlots() && getCountSlots() < SETTING(BAN_SLOTS)) ||
		int(getBytesShared() / (1024*1048576)) < SETTING(BAN_SHARE) ||
		(getLimit() && getLimit() < SETTING(BAN_LIMIT));
}


/**
 * @file
 * $Id: User.cpp,v 1.2 2007/10/16 13:33:37 alexey Exp $
 */
