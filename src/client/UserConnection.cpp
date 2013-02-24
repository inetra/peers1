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
//[-]PPA [Doxygen 1.5.1]  #include "DCPlusPlus.h"

#include "UserConnection.h"
//[-]PPA [Doxygen 1.5.1]  #include "ClientManager.h"
#include "ResourceManager.h"

#include "StringTokenizer.h"
//[-]PPA [Doxygen 1.5.1]  #include "AdcCommand.h"
//[-]PPA [Doxygen 1.5.1]  #include "DebugManager.h"

#include "FavoriteManager.h" // !SMT!-S

const string UserConnection::FEATURE_GET_ZBLOCK = "GetZBlock";
const string UserConnection::FEATURE_MINISLOTS = "MiniSlots";
const string UserConnection::FEATURE_XML_BZLIST = "XmlBZList";
const string UserConnection::FEATURE_ADCGET = "ADCGet";
const string UserConnection::FEATURE_ZLIB_GET = "ZLIG";
const string UserConnection::FEATURE_TTHL = "TTHL";
const string UserConnection::FEATURE_TTHF = "TTHF";
const string UserConnection::FEATURE_ADC_BASE = "BAS0";
const string UserConnection::FEATURE_ADC_BZIP = "BZIP";
const string UserConnection::FEATURE_BANMSG = "BanMsg"; // !SMT!-B

const string UserConnection::FILE_NOT_AVAILABLE = "File Not Available";

const string Transfer::TYPE_FILE = "file";
const string Transfer::TYPE_LIST = "list";
const string Transfer::TYPE_TTHL = "tthl";

const string Transfer::USER_LIST_NAME = "files.xml";
const string Transfer::USER_LIST_NAME_BZ = "files.xml.bz2";

const string UserConnection::UPLOAD = "Upload";
const string UserConnection::DOWNLOAD = "Download";

Transfer::Transfer(UserConnection& conn) : start(GET_TICK()), lastTick(GET_TICK()), runningAverage(0),
		last(0), actual(0), pos(0), startPos(0), size(-1), fileSize(-1), userConnection(conn) { }

void Transfer::updateRunningAverage() {
	uint64_t tick = GET_TICK();
	// Update 4 times/sec at most
	if(tick > (lastTick + 250)) {
		uint64_t diff = tick - lastTick;
		int64_t tot = getTotal();
		if( ((tick - getStart()) < AVG_PERIOD) ) {
			runningAverage = getAverageSpeed();
		} else {
			int64_t bdiff = tot - last;
			int64_t avg = bdiff * (int64_t)1000 / diff;
			if(diff > AVG_PERIOD) {
				runningAverage = avg;
			} else {
				// Weighted average...
				runningAverage = ((avg * diff) + (runningAverage*(AVG_PERIOD-diff)))/AVG_PERIOD;
			}
		}
		last = tot;
	}
	lastTick = tick;
}

void Transfer::getParams(const UserConnection& aSource, StringMap& params) {
	params["userNI"] = aSource.getUser()->getFirstNick();
	params["userI4"] = aSource.getRemoteIp();
	StringList hubNames = ClientManager::getInstance()->getHubNames(aSource.getUser()->getCID());
	if(hubNames.empty())
		hubNames.push_back(STRING(OFFLINE));
	params["hub"] = Util::toString(hubNames);
	StringList hubs = ClientManager::getInstance()->getHubs(aSource.getUser()->getCID());
	if(hubs.empty())
		hubs.push_back(STRING(OFFLINE));
	params["hubURL"] = Util::toString(hubs);
	params["fileSI"] = Util::toString(getSize());
	params["fileSIshort"] = Util::formatBytes(getSize());
	params["fileSIchunk"] = Util::toString(getTotal());
	params["fileSIchunkshort"] = Util::formatBytes(getTotal());
	params["fileSIactual"] = Util::toString(getActual());
	params["fileSIactualshort"] = Util::formatBytes(getActual());
	params["speed"] = Util::formatBytes(getAverageSpeed()) + "/s";
	params["time"] = Text::fromT(Util::formatSeconds((GET_TICK() - getStart()) / 1000));
	params["fileTR"] = getTTH().toBase32();
}

UserPtr Transfer::getUser() { return getUserConnection().getUser(); }

void UserConnection::on(BufferedSocketListener::Line, const string& aLine) throw () {

	if(aLine.length() < 2)
		return;

	COMMAND_DEBUG(aLine, DebugManager::CLIENT_IN, getRemoteIp());
	
	if(aLine[0] == 'C' && !isSet(FLAG_NMDC)) {
		dispatch(aLine);
		return;
	} else if(aLine[0] == '$') {
		setFlag(FLAG_NMDC);
	} else {
		// We shouldn't be here?
		if(getUser() && aLine.length() < 255)
			ClientManager::getInstance()->setUnknownCommand(getUser(), aLine);
		dcdebug("Unknown UserConnection command: %.50s\n", aLine.c_str());
		disconnect(true);
		return;
	}
	string cmd;
	string param;

	string::size_type x;
                
	if( (x = aLine.find(' ')) == string::npos) {
		cmd = aLine;
	} else {
		cmd = aLine.substr(0, x);
		param = aLine.substr(x+1);
    }
    
	if(cmd == PMNI) {
		if(!param.empty())
			fire(UserConnectionListener::MyNick(), this, Text::acpToUtf8(param));
	} else if(cmd == PDIR) {
		x = param.find(" ");
		if(x != string::npos) {
			fire(UserConnectionListener::Direction(), this, param.substr(0, x), param.substr(x+1));
		}
	} else if(cmd == "$Error") {
		if(Util::stricmp(param.c_str(), FILE_NOT_AVAILABLE) == 0 || 
			param.rfind(/*path/file*/" no more exists") != string::npos) { 
    		fire(UserConnectionListener::FileNotAvailable(), this);
                } else if (Util::strnicmp(param.c_str(), "BAN ", 4) == 0) { // !SMT!-B
                        fire(UserConnectionListener::BanMessage(), this, param); // !SMT!-B
    	} else {
			fire(UserConnectionListener::Failed(), this, param);
	    }
	} else if(cmd == PFIL) {
		if(!param.empty())
			fire(UserConnectionListener::FileLength(), this, Util::toInt64(param));
	} else if(cmd == PGLL) {
    	fire(UserConnectionListener::GetListLength(), this);
	} else if(cmd == PGET) {
		x = param.find('$');
		if(x != string::npos) {
			fire(UserConnectionListener::Get(), this, Text::acpToUtf8(param.substr(0, x)), Util::toInt64(param.substr(x+1)) - (int64_t)1);
	    }
	} else if(cmd == "$Key") {
		if(!param.empty())
			fire(UserConnectionListener::Key(), this, param);
	} else if(cmd == "$Lock") {
		if(!param.empty()) {
			x = param.find(" Pk=");
			if(x != string::npos) {
				fire(UserConnectionListener::CLock(), this, param.substr(0, x), param.substr(x + 4));
			} else {
				// Workaround for faulty linux clients...
				x = param.find(' ');
				if(x != string::npos) {
					fire(UserConnectionListener::CLock(), this, param.substr(0, x), Util::emptyString);
	    		} else {
					fire(UserConnectionListener::CLock(), this, param, Util::emptyString);
    			}
	        }
       	}
	} else if(cmd == PSND) {
    	fire(UserConnectionListener::Send(), this);
	} else if(cmd == PSND + "ing") {
		int64_t bytes = -1;
		if(!param.empty())
			bytes = Util::toInt64(param);
		fire(UserConnectionListener::Sending(), this, bytes);
	} else if(cmd == PMAX) {
		fire(UserConnectionListener::MaxedOut(), this, param);
	} else if(cmd == PSUP) {
		if(!param.empty()) {
			fire(UserConnectionListener::Supports(), this, StringTokenizer<string>(param, ' ').getTokens());
	    }
	} else if(cmd.compare(0, 4, "$ADC") == 0) {
    	dispatch(aLine, true);
	} else if (cmd == PLIL) {
		if(!param.empty()) {
			fire(UserConnectionListener::ListLength(), this, param);
		}
	} else {
		if(getUser() && aLine.length() < 255)
			ClientManager::getInstance()->setUnknownCommand(getUser(), aLine);
		
		dcdebug("Unknown NMDC command: %.50s\n", aLine.c_str());
	}
}

void UserConnection::connect(const string& aServer, uint16_t aPort) throw(SocketException, ThreadException) { 
	dcassert(!socket);

	socket = BufferedSocket::getSocket(0);
	socket->setDSCP(SETTING(PEER_DSCP_MARK));
	socket->addListener(this);
	socket->connect(aServer, aPort, 
#ifdef PPA_INCLUDE_SSL
		secure,
		BOOLSETTING(ALLOW_UNTRUSTED_CLIENTS),
#else
		false, 
		true,
#endif
		true);
}

void UserConnection::accept(const Socket& aServer) throw(SocketException, ThreadException) {
	dcassert(!socket);
	socket = BufferedSocket::getSocket(0);
	socket->setDSCP(SETTING(PEER_DSCP_MARK));
	socket->addListener(this);
	socket->accept(aServer, 
#ifdef PPA_INCLUDE_SSL
		secure, 
		BOOLSETTING(ALLOW_UNTRUSTED_CLIENTS),
#else
		false, 
		true
#endif
		);
}

void UserConnection::inf(bool withToken) { 
	AdcCommand c(AdcCommand::CMD_INF);
	c.addParam("ID", ClientManager::getInstance()->getMyCID().toBase32());
	if(withToken) {
		c.addParam("TO", getToken());
	}
	send(c);
}

void UserConnection::on(BufferedSocketListener::Failed, const string& aLine) throw() {
	setState(STATE_UNCONNECTED);
	fire(UserConnectionListener::Failed(), this, aLine);

	delete this;	
}

// !SMT!-S
void UserConnection::setUser(const UserPtr& aUser) {
        user = aUser;
	if (!socket) return;

        socket->setSuperUser(false);
        setUploadLimit(FavoriteUser::UL_NONE);

        if (!aUser) return;
        FavoriteManager::FavoriteMap favUsers = FavoriteManager::getInstance()->getFavoriteUsers();
        FavoriteManager::FavoriteMap::const_iterator i = favUsers.find(aUser->getCID());
        if(i == favUsers.end()) return;
        const FavoriteUser& u = i->second;

        socket->setSuperUser(u.isSet(FavoriteUser::FLAG_SUPERUSER));
        setUploadLimit(i->second.getUploadLimit());
}
void UserConnection::setUploadLimit(FavoriteUser::UPLOAD_LIMIT lim)
{
        switch (lim) {
           case FavoriteUser::UL_BAN:   disconnect(true);           break;
           case FavoriteUser::UL_SU:    socket->setSleep(-1);       break;
           case FavoriteUser::UL_2:     socket->setSleep(1000/2);   break;
           case FavoriteUser::UL_5:     socket->setSleep(1000/5);   break;
           case FavoriteUser::UL_8:     socket->setSleep(1000/8);   break;
           case FavoriteUser::UL_12:    socket->setSleep(1000/12);  break;
           case FavoriteUser::UL_16:    socket->setSleep(1000/16);  break;
           case FavoriteUser::UL_24:    socket->setSleep(1000/24);  break;
           case FavoriteUser::UL_32:    socket->setSleep(1000/32);  break;
           case FavoriteUser::UL_64:    socket->setSleep(1000/64);  break;
           case FavoriteUser::UL_128:   socket->setSleep(1000/128); break;
           case FavoriteUser::UL_256:   socket->setSleep(1000/256); break;
           default: socket->setSleep(0);
        }
}

/**
 * @file
 * $Id: UserConnection.cpp,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
