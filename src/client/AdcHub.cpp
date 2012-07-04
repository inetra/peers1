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

#include "AdcHub.h"
//[-]PPA [Doxygen 1.5.1] #include "ClientManager.h"
#include "ShareManager.h"
#include "StringTokenizer.h"
//[-]PPA [Doxygen 1.5.1] #include "AdcCommand.h"
#include "ConnectionManager.h"
//[-]PPA [Doxygen 1.5.1] #include "version.h"
//[-]PPA [Doxygen 1.5.1] #include "Util.h"
//[-]PPA [Doxygen 1.5.1] #include "UserCommand.h"
#include "FavoriteManager.h"
#include "CryptoManager.h"
#include "ResourceManager.h"
#include "LogManager.h"

const string AdcHub::CLIENT_PROTOCOL("ADC/0.10");
const string AdcHub::SECURE_CLIENT_PROTOCOL("ADCS/0.10");
const string AdcHub::ADCS_FEATURE("ADC0");
const string AdcHub::TCP4_FEATURE("TCP4");
const string AdcHub::UDP4_FEATURE("UDP4");

AdcHub::AdcHub(const string &clientId, const string& aHubURL, bool secure) : Client(clientId, aHubURL, '\n', secure), sid(0) {
	TimerManager::getInstance()->addListener(this);
}

AdcHub::~AdcHub() throw() {
	TimerManager::getInstance()->removeListener(this);
	clearUsers();
}


OnlineUser& AdcHub::getUser(const uint32_t aSID, const CID& aCID) {
	OnlineUser* ou = findUser(aSID);
	if(ou) {
		return *ou;
	}

	UserPtr p = ClientManager::getInstance()->getUser(aCID);

	{
		Lock l(cs);
		ou = users.insert(make_pair(aSID, new OnlineUser(p, *this, aSID))).first->second;
	}

	if(aSID != AdcCommand::HUB_SID)
		ClientManager::getInstance()->putOnline(ou);
	return *ou;
}

OnlineUser* AdcHub::findUser(const uint32_t aSID) const {
	Lock l(cs);
	SIDMap::const_iterator i = users.find(aSID);
	return i == users.end() ? NULL : i->second;
}

OnlineUser* AdcHub::findUser(const CID& aCID) const {
	Lock l(cs);
	for(SIDMap::const_iterator i = users.begin(); i != users.end(); ++i) {
		if(i->second->getUser()->getCID() == aCID) {
			return i->second;
		}
	}
	return 0;
}

void AdcHub::putUser(const uint32_t aSID) {
	OnlineUser* ou = 0;
	{
		Lock l(cs);
		SIDIter i = users.find(aSID);
		if(i == users.end())
			return;
		ou = i->second;
		users.erase(i);
	}

	if(aSID != AdcCommand::HUB_SID)
		ClientManager::getInstance()->putOffline(ou);
		
        fire(ClientListener::UserRemoved(), this, ou->getUser()); // !SMT!-fix
	delete ou;
}

void AdcHub::clearUsers() {
	SIDMap tmp;
	{
		Lock l(cs);
		users.swap(tmp);
	}

	for(SIDIter i = tmp.begin(); i != tmp.end(); ++i) {
		if(i->first != AdcCommand::HUB_SID)
			ClientManager::getInstance()->putOffline(i->second);
		delete i->second;
	}
}

void AdcHub::handle(AdcCommand::INF, AdcCommand& c) throw() {
	if(c.getParameters().empty())
		return;

	string cid;
	
	OnlineUser* u = 0;
	if(c.getParam("ID", 0, cid)) {
		u = findUser(CID(cid));
		if(u) {
			if(u->getIdentity().getSID() != c.getFrom()) {
				// Same CID but different SID not allowed - buggy hub?
				string nick;
				if(!c.getParam("NI", 0, nick)) {
					nick = "[nick unknown]";
				}
                                fire(ClientListener::Message(), this, (UserPtr)NULL, u->getIdentity().getNick() + " (" + u->getIdentity().getSIDString() +
					") has same CID {" + cid + "} as " + nick + " (" + AdcCommand::fromSID(c.getFrom()) + "), ignoring.");
				return;
			}
		} else {
			u = &getUser(c.getFrom(), CID(cid));
		}
	} else if(c.getFrom() == AdcCommand::HUB_SID) {
		u = &getUser(c.getFrom(), CID());
	} else {
		u = findUser(c.getFrom());
	}

	if(!u) {
		dcdebug("AdcHub::INF Unknown user / no ID\n");
		return;
	}

	for(StringIterC i = c.getParameters().begin(); i != c.getParameters().end(); ++i) {
		if(i->length() < 2)
			continue;
			
		u->getIdentity().set(i->c_str(), i->substr(2));
	}

	if(u->getIdentity().isBot()) {
		u->getUser()->setFlag(User::BOT);
	} else {
		u->getUser()->unsetFlag(User::BOT);
	}

	if(u->getUser()->getFirstNick().empty()) {
		u->getUser()->setFirstNick(u->getIdentity().getNick());
	}

	if(u->getIdentity().supports(ADCS_FEATURE)) {
		u->getUser()->setFlag(User::TLS);
	}

	if(u->getUser() == getMyIdentity().getUser()) {
		state = STATE_NORMAL;
		setAutoReconnect(true);
		setMyIdentity(u->getIdentity());
		updateCounts(false);
	}

	if(u->getIdentity().isHub()) {
		setHubIdentity(u->getIdentity());
		fire(ClientListener::HubUpdated(), this);
	} else {
                fire(ClientListener::UserUpdated(), this, u->getUser()); // !SMT!-fix
	}
}

void AdcHub::handle(AdcCommand::SUP, AdcCommand& c) throw() {
	if(state != STATE_PROTOCOL) /** @todo SUP changes */
		return;
	if(find(c.getParameters().begin(), c.getParameters().end(), "ADBASE") == c.getParameters().end()
		&& find(c.getParameters().begin(), c.getParameters().end(), "ADBAS0") == c.getParameters().end())
	{
                fire(ClientListener::Message(), this, (UserPtr)NULL, "Failed to negotiate base protocol"); // @todo internationalize
		socket->disconnect(false);
		return;
	}
}

void AdcHub::handle(AdcCommand::SID, AdcCommand& c) throw() {
	if(state != STATE_PROTOCOL) {
		dcdebug("Invalid state for SID\n");
		return;
	}

	if(c.getParameters().empty())
		return;

	sid = AdcCommand::toSID(c.getParam(0));

	state = STATE_IDENTIFY;
	info();
}

void AdcHub::handle(AdcCommand::MSG, AdcCommand& c) throw() {
	if(c.getParameters().empty())
		return;

	OnlineUser* from = findUser(c.getFrom());
	if(!from)
		return;

	string msg = '<' + from->getIdentity().getNick() + "> " + c.getParam(0);
	string pmFrom;
	if(c.getParam("PM", 1, pmFrom)) { // add PM<group-cid> as well
		OnlineUser* to = findUser(c.getTo());
		if(!to)
			return;

		OnlineUser* replyTo = findUser(AdcCommand::toSID(pmFrom));
		if(!replyTo)
			return;

                fire(ClientListener::PrivateMessage(), this, from->getUser(), to->getUser(), replyTo->getUser(), msg); // !SMT!-fix
	} else {
                fire(ClientListener::Message(), this, from->getUser(), msg);
	}		
}

void AdcHub::handle(AdcCommand::GPA, AdcCommand& c) throw() {
	if(c.getParameters().empty())
		return;
	salt = c.getParam(0);
	state = STATE_VERIFY;

	fire(ClientListener::GetPassword(), this);
}

void AdcHub::handle(AdcCommand::QUI, AdcCommand& c) throw() {
	uint32_t s = AdcCommand::toSID(c.getParam(0));
	putUser(s);

	// No use to hammer if we're banned
	if(s == sid &&  c.hasFlag("TL", 1)) {
		setAutoReconnect(false);
	}
}

void AdcHub::handle(AdcCommand::CTM, AdcCommand& c) throw() {
	OnlineUser* u = findUser(c.getFrom());
	if(!u || u->getUser() == ClientManager::getInstance()->getMe())
		return;
	if(c.getParameters().size() < 3)
		return;

	const string& protocol = c.getParam(0);
	const string& port = c.getParam(1);

	string token;
	bool hasToken = c.getParam("TO", 2, token);

	bool secure;
	if(protocol == CLIENT_PROTOCOL) {
		secure = false;
	} else if(protocol == SECURE_CLIENT_PROTOCOL && CryptoManager::getInstance()->TLSOk()) {
		secure = true;
	} else {
		AdcCommand cmd(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_UNSUPPORTED, "Protocol unknown");
		cmd.setTo(c.getFrom());
		cmd.addParam("PR", protocol);

		if(hasToken)
			cmd.addParam("TO", token);

		send(cmd);
		return;
	}

	if(!u->getIdentity().isTcpActive()) {
		send(AdcCommand(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_GENERIC, "IP unknown", AdcCommand::TYPE_DIRECT).setTo(c.getFrom()));
		return;
	}

	ConnectionManager::getInstance()->adcConnect(*u, (short)Util::toInt(port), token, secure);
}

void AdcHub::handle(AdcCommand::RCM, AdcCommand& c) throw() {
	if(c.getParameters().empty()) {
		return;
	}
	if(!isActive())
		return;
	OnlineUser* u = findUser(c.getFrom());
	if(!u || u->getUser() == ClientManager::getInstance()->getMe())
		return;

	const string& protocol = c.getParam(0);
	string token;
	bool hasToken = c.getParam("TO", 1, token);

	bool secure;
	if(protocol == CLIENT_PROTOCOL) {
		secure = false;
	} else if(protocol == SECURE_CLIENT_PROTOCOL && CryptoManager::getInstance()->TLSOk()) {
		secure = true;
	} else {
		AdcCommand cmd(AdcCommand::SEV_FATAL, AdcCommand::ERROR_PROTOCOL_UNSUPPORTED, "Protocol unknown");
		cmd.setTo(c.getFrom());
		cmd.addParam("PR", protocol);
		
		if(hasToken)
			cmd.addParam("TO", token);

		send(cmd);
		return;
	}
        connect(*u, token, secure);
}

void AdcHub::handle(AdcCommand::CMD, AdcCommand& c) throw() {
	if(c.getParameters().size() < 1)
		return;
	const string& name = c.getParam(0);
	bool rem = c.hasFlag("RM", 1);
	if(rem) {
		int cmd = FavoriteManager::getInstance()->findUserCommand(name);
		if(cmd != -1)
			FavoriteManager::getInstance()->removeUserCommand(cmd);
	}
	bool sep = c.hasFlag("SP", 1);
	string sctx;
	if(!c.getParam("CT", 1, sctx))
		return;
	int ctx = Util::toInt(sctx);
	if(ctx <= 0)
		return;
	if(sep) {
		FavoriteManager::getInstance()->addUserCommand(UserCommand::TYPE_SEPARATOR, ctx, UserCommand::FLAG_NOSAVE, name, "", getHubUrl());
		return;
	}
	bool once = c.hasFlag("CO", 1);
	string txt;
	if(!c.getParam("TT", 1, txt))
		return;
	FavoriteManager::getInstance()->addUserCommand(once ? UserCommand::TYPE_RAW_ONCE : UserCommand::TYPE_RAW, ctx, UserCommand::FLAG_NOSAVE, name, txt, getHubUrl());
}

void AdcHub::sendUDP(const AdcCommand& cmd) throw() {
	string command;
	string ip;
	uint16_t port;
	{
		Lock l(cs);
		SIDMap::const_iterator i = users.find(cmd.getTo());
		if(i == users.end()) {
			dcdebug("AdcHub::sendUDP: invalid user\n");
			return;
		}
		OnlineUser& ou = *i->second;
		if(!ou.getIdentity().isUdpActive()) {
			return;
		}
		ip = ou.getIdentity().getIp();
		port = static_cast<uint16_t>(Util::toInt(ou.getIdentity().getUdpPort()));
		command = cmd.toString(ou.getUser()->getCID());
	}
	try {
		udp.writeTo(ip, port, command);
	} catch(const SocketException& e) {
		dcdebug("AdcHub::sendUDP: write failed: %s\n", e.getError().c_str());
	}
}

void AdcHub::handle(AdcCommand::STA, AdcCommand& c) throw() {
	if(c.getParameters().size() < 2)
		return;

	OnlineUser* u = findUser(c.getFrom());
	if(!u)
		return;

	//int severity = Util::toInt(c.getParam(0).substr(0, 1));
	if(c.getParam(0).size() != 3) {
		return;
	}

	int code = Util::toInt(c.getParam(0).substr(1));

	if(code == AdcCommand::ERROR_BAD_PASSWORD) {
		setPassword(Util::emptyString);
	}
	// @todo Check for invalid protocol and unset TLS if necessary
        fire(ClientListener::Message(), this, u->getUser(), c.getParam(1));
}

void AdcHub::handle(AdcCommand::SCH, AdcCommand& c) throw() {	
	OnlineUser* ou = findUser(c.getFrom());
	if(!ou) {
		dcdebug("Invalid user in AdcHub::onSCH\n");
		return;
	}

	fire(ClientListener::AdcSearch(), this, c, ou->getUser()->getCID());
}

void AdcHub::handle(AdcCommand::RES, AdcCommand& c) throw() {
	OnlineUser* ou = findUser(c.getFrom());
	if(!ou) {
		dcdebug("Invalid user in AdcHub::onRES\n");
		return;
	}
	SearchManager::getInstance()->onRES(c, ou->getUser());
}

void AdcHub::connect(const OnlineUser& user) {
	uint32_t r = Util::rand();
	connect(user, Util::toString(r), CryptoManager::getInstance()->TLSOk() && user.getUser()->isSet(User::TLS));
}

void AdcHub::connect(const OnlineUser& user, string const& token, bool secure) {
	if(state != STATE_NORMAL)
		return;

	const string& proto = secure ? SECURE_CLIENT_PROTOCOL : CLIENT_PROTOCOL;
	if(isActive()) {
		uint16_t port = secure ? ConnectionManager::getInstance()->getSecurePort() : ConnectionManager::getInstance()->getPort();
		if(port == 0) {
			// Oops?
			LogManager::getInstance()->message(STRING(NOT_LISTENING));
			return;
		}
		send(AdcCommand(AdcCommand::CMD_CTM, user.getIdentity().getSID(), AdcCommand::TYPE_DIRECT).addParam(proto).addParam(Util::toString(port)).addParam(token));
	} else {
		send(AdcCommand(AdcCommand::CMD_RCM, user.getIdentity().getSID(), AdcCommand::TYPE_DIRECT).addParam(proto));
	}
}

void AdcHub::hubMessage(const string& aMessage) {
	if(state != STATE_NORMAL)
		return;
	send(AdcCommand(AdcCommand::CMD_MSG, AdcCommand::TYPE_BROADCAST).addParam(aMessage)); 
}

void AdcHub::privateMessage(const UserPtr& user, const string& aMessage, bool) {
	if(state != STATE_NORMAL)
		return;
        // !SMT!-S
        uint32_t sid = 0;
	{
    	ClientManager::LockInstance l_instance;
        if (const OnlineUser* ou = ClientManager::getInstance()->getOnlineUser(user)) 
	     sid = ou->getIdentity().getSID();
        else
	  return;
	}
        send(AdcCommand(AdcCommand::CMD_MSG, sid, AdcCommand::TYPE_ECHO).addParam(aMessage).addParam("PM", getMySID()));
}

void AdcHub::search(int aSizeMode, int64_t aSize, int aFileType, const string& aString, const string& aToken) { 
	if(state != STATE_NORMAL)
		return;

	AdcCommand c(AdcCommand::CMD_SCH, AdcCommand::TYPE_BROADCAST);

	if(aFileType == SearchManager::TYPE_TTH) {
		c.addParam("TR", aString);
	} else {
	if(aSizeMode == SearchManager::SIZE_ATLEAST) {
			c.addParam("GE", Util::toString(aSize));
	} else if(aSizeMode == SearchManager::SIZE_ATMOST) {
			c.addParam("LE", Util::toString(aSize));
	}
		StringTokenizer<string> st(aString, ' ');
	for(StringIter i = st.getTokens().begin(); i != st.getTokens().end(); ++i) {
			c.addParam("AN", *i);
		}
		if(aFileType == SearchManager::TYPE_DIRECTORY) {
			c.addParam("TY", "2");
		}
	}

	if(!aToken.empty())
		c.addParam("TO", aToken);

	if(isActive()) {
		send(c);
	} else {
		c.setType(AdcCommand::TYPE_FEATURE);
		c.setFeatures("+TCP4");
		send(c);
	}
}

void AdcHub::password(const string& pwd) { 
	if(state != STATE_VERIFY)
		return;
	if(!salt.empty()) {
		size_t saltBytes = salt.size() * 5 / 8;
		AutoArray<uint8_t> buf(saltBytes);
		Encoder::fromBase32(salt.c_str(), buf, saltBytes);
		TigerHash th;
		CID cid = getMyIdentity().getUser()->getCID();
		th.update(cid.data(), CID::SIZE);
		th.update(pwd.data(), pwd.length());
		th.update(buf, saltBytes);
		send(AdcCommand(AdcCommand::CMD_PAS, AdcCommand::TYPE_HUB).addParam(Encoder::toBase32(th.finalize(), TigerHash::HASH_SIZE)));
		salt.clear();
	}
}

void AdcHub::info() {
	if(state != STATE_IDENTIFY && state != STATE_NORMAL)
		return;

	reloadSettings(false);

	AdcCommand c(AdcCommand::CMD_INF, AdcCommand::TYPE_BROADCAST);
	string tmp;

	StringMapIter i;
#define ADDPARAM(var, content) \
	tmp = content; \
	if((i = lastInfoMap.find(var)) != lastInfoMap.end()) { \
		if(i->second != tmp) { \
			if(tmp.empty()) \
				lastInfoMap.erase(i); \
			else \
				i->second = tmp; \
			c.addParam(var, tmp); \
		} \
	} else if(!tmp.empty()) { \
		c.addParam(var, tmp); \
		lastInfoMap[var] = tmp; \
	}

	updateCounts(false); \
	
	ADDPARAM("ID", ClientManager::getInstance()->getMyCID().toBase32());
	ADDPARAM("PD", ClientManager::getInstance()->getMyPID().toBase32());
	ADDPARAM("NI", getCurrentNick());
	ADDPARAM("DE", getCurrentDescription());
	ADDPARAM("SL", Util::toString(SETTING(SLOTS)));
	ADDPARAM("SS", ShareManager::getInstance()->getShareSizeString());
	ADDPARAM("SF", Util::toString(ShareManager::getInstance()->getSharedFiles()));
	ADDPARAM("EM", SETTING(EMAIL));
	ADDPARAM("HN", Util::toString(counts.normal));
	ADDPARAM("HR", Util::toString(counts.registered));
	ADDPARAM("HO", Util::toString(counts.op));
	ADDPARAM("VE", "++ " DCVERSIONSTRING);
	ADDPARAM("US", Util::toString((long)(Util::toDouble(SETTING(UPLOAD_SPEED))*1024*1024)));

	if(SETTING(MAX_DOWNLOAD_SPEED) > 0) {
		ADDPARAM("DS", Util::toString((SETTING(MAX_DOWNLOAD_SPEED)*1024*8)));
	} else {
		ADDPARAM("DS", Util::emptyString);
	}

	if(Util::getAway()){
		ADDPARAM("AW", '1');
	} else {
		ADDPARAM("AW", Util::emptyString);
	}

	string su;
	if(CryptoManager::getInstance()->TLSOk()) {
		su += ADCS_FEATURE + ",";
	}
	
	if(isActive()) {
		if(BOOLSETTING(NO_IP_OVERRIDE) && !SETTING(EXTERNAL_IP).empty()) {
			ADDPARAM("I4", Socket::resolve(SETTING(EXTERNAL_IP)));
		} else {
			ADDPARAM("I4", "0.0.0.0");
		}
		ADDPARAM("U4", Util::toString(SearchManager::getInstance()->getPort()));
		su += TCP4_FEATURE + ",";
		su += UDP4_FEATURE + ",";
	} else {
		ADDPARAM("I4", "");
		ADDPARAM("U4", "");
	}

	if(!su.empty()) {
		su.erase(su.size() - 1);
	}
	ADDPARAM("SU", su);

#undef ADDPARAM

	if(c.getParameters().size() > 0) {
		send(c);
	}
}

string AdcHub::checkNick(const string& aNick) {
	string tmp = aNick;
	for(size_t i = 0; i < aNick.size(); ++i) {
		if(static_cast<uint8_t>(tmp[i]) <= 32) {
			tmp[i] = '_';
		}
	}
	return tmp;
}

void AdcHub::send(const AdcCommand& cmd) {
	if(cmd.getType() == AdcCommand::TYPE_UDP)
		sendUDP(cmd);
	send(cmd.toString(sid));
}

void AdcHub::on(Connected) throw() { 
	Client::on(Connected());

	lastInfoMap.clear();
	sid = 0;

	send(AdcCommand(AdcCommand::CMD_SUP, AdcCommand::TYPE_HUB).addParam("ADBAS0"));
}

void AdcHub::on(Line, const string& aLine) throw() {
	Client::on(Line(), aLine);	
	if(BOOLSETTING(ADC_DEBUG)) {
                fire(ClientListener::Message(), this, (UserPtr)NULL, "<ADC>" + aLine + "</ADC>");
	}
	dispatch(aLine); 
}

void AdcHub::on(Failed, const string& aLine) throw() { 
	clearUsers();
	Client::on(Failed(), aLine);
}

void AdcHub::on(Second, uint32_t aTick) throw() {
	Client::on(Second(), aTick);
	if(state == STATE_NORMAL && (aTick > (getLastActivity() + 120*1000)) ) {
		send("\n", 1);
	}
}

/**
 * @file
 * $Id: AdcHub.cpp,v 1.2 2007/10/15 10:54:30 alexey Exp $
 */
