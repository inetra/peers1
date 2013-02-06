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

//[-]PPA [Doxygen 1.5.1] #include "Client.h"

//[-]PPA [Doxygen 1.5.1] #include "BufferedSocket.h"

#include "FavoriteManager.h"
#include "TimerManager.h"
#include "ResourceManager.h"
//[-]PPA [Doxygen 1.5.1] #include "ClientManager.h"
#include "UploadManager.h"

Client::Counts Client::counts;

Client::Client(const string& clientId, const string& hubURL, char separator_, bool secure_) : 
        m_clientId(clientId),
	myIdentity(ClientManager::getInstance()->getMe(), 0),
	reconnDelay(120), lastActivity(GET_TICK()), registered(false), autoReconnect(false), state(STATE_DISCONNECTED), socket(0), 
	originalHubUrl(hubURL), hubUrl(hubURL), port(0), separator(separator_),
	secure(secure_), countType(COUNT_UNCOUNTED), availableBytes(0)
{
	TimerManager::getInstance()->addListener(this);
}

Client::~Client() throw() {
	dcassert(!socket);
	TimerManager::getInstance()->removeListener(this);
	updateCounts(true);
}

void Client::redirectToURL(const std::string& redirectUrl) throw() {
	disconnect(false);
	hubUrl = redirectUrl;
	connect();
}


void Client::reconnect() {
	disconnect(true);
	setAutoReconnect(true);
	setReconnDelay(0);
}

void Client::shutdown() {

	if(socket) {
		BufferedSocket::putSocket(socket);
		socket = 0;
	}
}

void Client::reloadSettings(bool updateNick) {
	const FavoriteHubEntry* hub = FavoriteManager::getInstance()->getFavoriteHubEntry(getHubUrl());
	
	string speedDescription = Util::emptyString;

#ifdef BETA
	if(!SETTING(BETAUSR).empty())
		speedDescription += "[Auth:" + SETTING(BETAUSR) + "]";
#endif

	if (getStealth() == false) {
		if(BOOLSETTING(SHOW_DESCRIPTION_SLOTS))
			speedDescription += "[" + Util::toString(UploadManager::getInstance()->getFreeSlots()) + "]";
		if(BOOLSETTING(SHOW_DESCRIPTION_LIMIT) && SETTING(THROTTLE_ENABLE) && SETTING(MAX_UPLOAD_SPEED_LIMIT) != 0)
			speedDescription += "[L:" + Util::toString(SETTING(MAX_UPLOAD_SPEED_LIMIT)) + "KB]";
	}

        // !SMT!-S
        string ClientId;
        if (getStealth()) 
			ClientId = "++ V:" DCVERSIONSTRING;
        else 
			ClientId = m_clientId;

	if(hub) {
		if(updateNick) {
			setCurrentNick(checkNick(hub->getNick(true)));
		}		

		if(!hub->getUserDescription().empty()) {
			setCurrentDescription(speedDescription + hub->getUserDescription());
		} else {
			setCurrentDescription(speedDescription + SETTING(DESCRIPTION));
		}
		if(!hub->getEmail().empty()) {
			setCurrentEmail(hub->getEmail());
		} else {
			setCurrentEmail(SETTING(EMAIL));
		}
		if(!hub->getPassword().empty())
			setPassword(hub->getPassword());
		setStealth(hub->getStealth());
		if(Util::strnicmp("adc://", hub->getServer().c_str(), 6) == 0 || Util::strnicmp("adcs://", hub->getServer().c_str(), 7) == 0) {
			setHideShare(false); // Hide Share Mod
		} else {
			setHideShare(hub->getHideShare()); // Hide Share Mod
		}
		setFavIp(hub->getIP());
                if (hub->getOverrideId()) ClientId = hub->getClientId(); // !SMT!-S
	} else {
		if(updateNick) {
			setCurrentNick(checkNick(SETTING(NICK)));
		}
		setCurrentDescription(speedDescription + SETTING(DESCRIPTION));
		setCurrentEmail(SETTING(EMAIL));
		setStealth(true);
		setHideShare(false); // Hide Share Mod
		setFavIp(Util::emptyString);
	}
        // !SMT!-S
        for (unsigned i = 0; i < ClientId.length(); i++)
                if (ClientId[i] == '<' || ClientId[i] == '>' || ClientId[i] == ',' || ClientId[i] == '$' || ClientId[i] == '|') {
                        ClientId = ClientId.substr(0, i);
                        break;
                }
        setClientId(ClientId);
}

bool Client::isActive() const { return ClientManager::getInstance()->getMode(hubUrl) != SettingsManager::INCOMING_FIREWALL_PASSIVE; }

void Client::connect() {
	if(socket)
		BufferedSocket::putSocket(socket);

	// TODO should this be done also on disconnect ???
	FavoriteManager::getInstance()->removeUserCommand(getHubUrl());
	availableBytes = 0;

	setAutoReconnect(true);
	setReconnDelay(120 + Util::rand(0, 60));
	reloadSettings(true);
	setRegistered(false);
	setMyIdentity(Identity(ClientManager::getInstance()->getMe(), 0));
	setHubIdentity(Identity());

	try {
		socket = BufferedSocket::getSocket(separator);
		socket->addListener(this);
		string file;
		Util::decodeUrl(hubUrl, address, port, file);

		socket->connect(address, port, secure, BOOLSETTING(ALLOW_UNTRUSTED_HUBS), true);
	} catch(const Exception& e) {
		if(socket) {
			BufferedSocket::putSocket(socket);
			socket = 0;
		}
		fire(ClientListener::Failed(), this, e.getError());
	}
	updateActivity();
	state = STATE_CONNECTING;
}

void Client::on(Connected) throw() {
	updateActivity(); 
	ip = socket->getIp(); 
	fire(ClientListener::Connected(), this);
	state = STATE_PROTOCOL;
}

void Client::on(Failed, const string& aLine) throw() {
	updateActivity(); //[+]StrongDC++2.05
	state = STATE_DISCONNECTED;
	socket->removeListener(this);
	fire(ClientListener::Failed(), this, aLine);
}

void Client::disconnect(bool graceLess) {
	if(socket) 
		socket->disconnect(graceLess);
}

void Client::updateCounts(bool aRemove) {
	// We always remove the count and then add the correct one if requested...
	if(countType == COUNT_NORMAL) {
		Thread::safeDec(counts.normal);
	} else if(countType == COUNT_REGISTERED) {
		Thread::safeDec(counts.registered);
	} else if(countType == COUNT_OP) {
		Thread::safeDec(counts.op);
	}

	countType = COUNT_UNCOUNTED;

	if(!aRemove) {
		if(getMyIdentity().isOp()) {
			Thread::safeInc(counts.op);
			countType = COUNT_OP;
		} else if(getMyIdentity().isRegistered()) {
			Thread::safeInc(counts.registered);
			countType = COUNT_REGISTERED;
		} else {
			Thread::safeInc(counts.normal);
			countType = COUNT_NORMAL;
		}
	}
}

string Client::getLocalIp() const {
	// Favorite hub Ip
	if(!getFavIp().empty())
		return getFavIp();

	// Best case - the server detected it
	if((!BOOLSETTING(NO_IP_OVERRIDE) || SETTING(EXTERNAL_IP).empty()) && !getMyIdentity().getIp().empty()) {
		return getMyIdentity().getIp();
	}

        
        if(!SETTING(EXTERNAL_IP).empty() 
            &&
            SETTING(INCOMING_CONNECTIONS) != SettingsManager::INCOMING_DIRECT )  // !SMT!-F
        { 
		return Socket::resolve(SETTING(EXTERNAL_IP));
	}

	string lip;
	if(socket)
		lip = socket->getLocalIp();

	if(lip.empty())
		return Util::getLocalIp();
	return lip;
}

void Client::on(Line, const string& aLine) throw() {
	updateActivity();
	COMMAND_DEBUG(aLine, DebugManager::HUB_IN, getIpPort());
}

void Client::on(Second, uint32_t aTick) throw() {
	if(state == STATE_DISCONNECTED && getAutoReconnect() && (aTick > (getLastActivity() + getReconnDelay() * 1000)) ) {
		// Try to reconnect...
		hubUrl = originalHubUrl;
		connect();
	}
}

// !SMT!-S
OnlineUser* Client::getUser(const UserPtr& aUser)
{
        // for generic client, use ClientManager, but it does not correctly handle ClientManager::me
        return ClientManager::getInstance()->getOnlineUser(aUser);
}


/**
 * @file
 * $Id: Client.cpp,v 1.3 2007/10/15 10:54:30 alexey Exp $
 */
