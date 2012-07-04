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

#ifndef DCPLUSPLUS_CLIENT_USER_CONNECTION_H
#define DCPLUSPLUS_CLIENT_USER_CONNECTION_H

#include "TimerManager.h"

#include "BufferedSocket.h"
#include "CriticalSection.h"
#include "File.h"
#include "User.h"
#include "AdcCommand.h"
#include "MerkleTree.h"
#include "DebugManager.h"
#include "ClientManager.h"
#include "FavoriteUser.h" // !SMT!-S

class UserConnection;

class UserConnectionListener {
public:
	virtual ~UserConnectionListener() { }
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> BytesSent;
	typedef X<1> Connected;
	typedef X<2> Data;
	typedef X<3> Failed;
	typedef X<4> CLock;
	typedef X<5> Key;
	typedef X<6> Direction;
	typedef X<7> Get;
	typedef X<8> Error;
	typedef X<10> Sending;
	typedef X<11> FileLength;
	typedef X<12> Send;
	typedef X<13> GetListLength;
	typedef X<14> MaxedOut;
	typedef X<15> ModeChange;
	typedef X<16> MyNick;
	typedef X<17> TransmitDone;
	typedef X<18> Supports;
	typedef X<19> FileNotAvailable;
	typedef X<20> ADCGet;
	typedef X<21> ADCSnd;
	typedef X<22> ADCSta;
	typedef X<23> ListLength; 
        typedef X<24> BanMessage; // !SMT!-B

	virtual void on(BytesSent, UserConnection*, size_t, size_t) throw() { }
	virtual void on(Connected, UserConnection*) throw() { }
	virtual void on(Data, UserConnection*, const uint8_t*, size_t) throw() { }
	virtual void on(Error, UserConnection*, const string&) throw() { }
	virtual void on(Failed, UserConnection*, const string&) throw() { }
	virtual void on(CLock, UserConnection*, const string&, const string&) throw() { }
	virtual void on(Key, UserConnection*, const string&) throw() { }
	virtual void on(Direction, UserConnection*, const string&, const string&) throw() { }
	virtual void on(Get, UserConnection*, const string&, int64_t) throw() { }
	virtual void on(Sending, UserConnection*, int64_t) throw() { }
	virtual void on(FileLength, UserConnection*, int64_t) throw() { }
	virtual void on(Send, UserConnection*) throw() { }
	virtual void on(GetListLength, UserConnection*) throw() { }
        virtual void on(MaxedOut, UserConnection*, string param = Util::emptyString) throw() { }
	virtual void on(ModeChange, UserConnection*) throw() { }
	virtual void on(MyNick, UserConnection*, const string&) throw() { }
	virtual void on(TransmitDone, UserConnection*) throw() { }
	virtual void on(Supports, UserConnection*, const StringList&) throw() { }
	virtual void on(FileNotAvailable, UserConnection*) throw() { }
	virtual void on(ListLength, UserConnection*, const string&) throw() { }
        virtual void on(BanMessage, UserConnection*, const string&) throw() { }  // !SMT!-B

	virtual void on(AdcCommand::SUP, UserConnection*, const AdcCommand&) throw() { }
	virtual void on(AdcCommand::INF, UserConnection*, const AdcCommand&) throw() { }
	virtual void on(AdcCommand::GET, UserConnection*, const AdcCommand&) throw() { }
	virtual void on(AdcCommand::SND, UserConnection*, const AdcCommand&) throw() { }
	virtual void on(AdcCommand::STA, UserConnection*, const AdcCommand&) throw() { }
	virtual void on(AdcCommand::RES, UserConnection*, const AdcCommand&) throw() { }
	virtual void on(AdcCommand::GFI, UserConnection*, const AdcCommand&) throw() { }
};

class ConnectionQueueItem;

class Transfer {
public:
	static const string TYPE_FILE;		///< File transfer
	static const string TYPE_LIST;		///< Partial file list
	static const string TYPE_TTHL;		///< TTH Leaves

	static const string USER_LIST_NAME;
	static const string USER_LIST_NAME_BZ;

	Transfer(UserConnection& conn);
	virtual ~Transfer() { };
	
	int64_t getPos() const { return pos; }
	void setPos(int64_t aPos) { pos = aPos; }

	void resetPos() { pos = getStartPos(); }
	void setStartPos(int64_t aPos) { startPos = aPos; pos = aPos; }
	int64_t getStartPos() const { return startPos; }

	void addPos(int64_t aBytes, int64_t aActual) { pos += aBytes; actual+= aActual; }

	enum { AVG_PERIOD = 30000 };
	void updateRunningAverage();

	int64_t getTotal() const { return getPos() - getStartPos(); }
	int64_t getActual() const { return actual; }
	
	int64_t getSize() const { return size; }
	void setSize(int64_t aSize) { size = aSize; }

	int64_t getAverageSpeed() const {
		int64_t diff = (int64_t)(GET_TICK() - getStart());
		return (diff > 0) ? (getTotal() * (int64_t)1000 / diff) : 0;
	}

	int64_t getSecondsLeft(bool wholeFile = false) {
		updateRunningAverage();
		int64_t avg = getRunningAverage();
		return (avg > 0) ? (((wholeFile ? getFileSize() : getSize()) - getPos()) / avg) : 0;
	}

	int64_t getBytesLeft() const {
		return getSize() - getPos();
	}

	virtual void getParams(const UserConnection& aSource, StringMap& params);

	UserPtr getUser();

	UserConnection& getUserConnection() { return userConnection; }
	const UserConnection& getUserConnection() const { return userConnection; }

	GETSET(uint64_t, start, Start);
	GETSET(uint64_t, lastTick, LastTick);
	GETSET(int64_t, runningAverage, RunningAverage);
	GETSET(TTHValue, tth, TTH);	
	GETSET(int64_t, fileSize, FileSize);
private:
	Transfer(const Transfer&);
	Transfer& operator=(const Transfer&);
	
	/** Bytes on last avg update */
	int64_t last;
	/** Total actual bytes transfered this session (compression?) */
	int64_t actual;
	/** Write position in file */
	int64_t pos;
	/** Starting position */
	int64_t startPos;
	/** Target size of this transfer */
	int64_t size;

	UserConnection& userConnection;
};

class ServerSocket;
class Upload;
class Download;

#ifdef CAM
	static const string PGLL = "$TEGListLen";
	static const string PLIL = "$SILtLen";
	static const string PMNI = "$NYMick";
	static const string PMAX = "$XAMedOut";
	static const string PSUP = "$PUSports";
	static const string PFIL = "$LIFeLength";
	static const string PGET = "$TEG";
	static const string PSND = "$NESd";
	static const string PCNC = "$NACceled";
	static const string PDIR = "$RIDection";
#else
	static const string PGLL = "$GetListLen";
	static const string PLIL = "$ListLen";
	static const string PMNI = "$MyNick";
	static const string PMAX = "$MaxedOut";
	static const string PSUP = "$Supports";
	static const string PFIL = "$FileLength";
	static const string PGET = "$Get";
	static const string PSND = "$Send";
	static const string PCNC = "$Canceled";
	static const string PDIR = "$Direction";
#endif

class UserConnection : public Speaker<UserConnectionListener>, 
	private BufferedSocketListener, public Flags, private CommandHandler<UserConnection>
{
public:
	friend class ConnectionManager;
	
	typedef UserConnection* Ptr;
	typedef FlyLinkVector<Ptr> List;
	typedef List::const_iterator Iter;

	static const string FEATURE_GET_ZBLOCK;
	static const string FEATURE_MINISLOTS;
	static const string FEATURE_XML_BZLIST;
	static const string FEATURE_ADCGET;
	static const string FEATURE_ZLIB_GET;
	static const string FEATURE_TTHL;
	static const string FEATURE_TTHF;
	static const string FEATURE_ADC_BASE;
	static const string FEATURE_ADC_BZIP;
        static const string FEATURE_BANMSG; // !SMT!-B

	static const string FILE_NOT_AVAILABLE;

	enum Modes {	
		MODE_COMMAND = BufferedSocket::MODE_LINE,
		MODE_DATA = BufferedSocket::MODE_DATA
	};

	enum Flags {
		FLAG_NMDC = 0x01,
		FLAG_OP = FLAG_NMDC << 1,
		FLAG_UPLOAD = FLAG_OP << 1,
		FLAG_DOWNLOAD = FLAG_UPLOAD << 1,
		FLAG_INCOMING = FLAG_DOWNLOAD << 1,
		FLAG_ASSOCIATED = FLAG_INCOMING << 1,
		FLAG_HASSLOT = FLAG_ASSOCIATED << 1,
		FLAG_HASEXTRASLOT = FLAG_HASSLOT << 1,
		FLAG_INVALIDKEY = FLAG_HASEXTRASLOT << 1,
		FLAG_SUPPORTS_GETZBLOCK = FLAG_INVALIDKEY << 1,
		FLAG_SUPPORTS_MINISLOTS = FLAG_SUPPORTS_GETZBLOCK << 1,
		FLAG_SUPPORTS_XML_BZLIST = FLAG_SUPPORTS_MINISLOTS << 1,
		FLAG_SUPPORTS_ADCGET = FLAG_SUPPORTS_XML_BZLIST << 1,
		FLAG_SUPPORTS_ZLIB_GET = FLAG_SUPPORTS_ADCGET << 1,
		FLAG_SUPPORTS_TTHL = FLAG_SUPPORTS_ZLIB_GET << 1,
		FLAG_SUPPORTS_TTHF = FLAG_SUPPORTS_TTHL << 1,
                FLAG_STEALTH = FLAG_SUPPORTS_TTHF << 1,
                FLAG_SUPPORTS_BANMSG = FLAG_STEALTH << 1 // !SMT!-S
	};
	
	enum States {
		// ConnectionManager
		STATE_UNCONNECTED,
		STATE_CONNECT,

		// Handshake
		STATE_SUPNICK,		// ADC: SUP, Nmdc: $Nick
		STATE_INF,
		STATE_LOCK,
		STATE_DIRECTION,
		STATE_KEY,

		// UploadManager
		STATE_GET,			// Waiting for GET
		STATE_SEND,			// Waiting for $Send
		STATE_RUNNING,		// Transmitting data

		// DownloadManager
		STATE_FILELENGTH,
		STATE_TREE

	};

	short getNumber() const { return (short)((((size_t)this)>>2) & 0x7fff); }

	// NMDC stuff
	void myNick(const string& aNick) { send(PMNI + " " + Text::utf8ToAcp(aNick) + '|'); }
	void lock(const string& aLock, const string& aPk) { send ("$Lock " + aLock + " Pk=" + aPk + '|'); }
	void key(const string& aKey) { send("$Key " + aKey + '|'); }
	void direction(const string& aDirection, int aNumber) { send(PDIR + " " + aDirection + " " + Util::toString(aNumber) + '|'); }
	void get(const string& aFile, int64_t aResume) { send(PGET + " " + aFile + "$" + Util::toString(aResume + 1) + '|'); } 	// No acp - utf conversion here...
	void fileLength(const string& aLength) { send(PFIL + " " + aLength + '|'); }
	void startSend() { send(PSND + "|"); }
	void sending(int64_t bytes) { send(bytes == -1 ? string(PSND + "ing|") : PSND + "ing " + Util::toString(bytes) + "|"); }
	void error(const string& aError) { send("$Error " + aError + '|'); }
	void listLen(const string& aLength) { send(PLIL + " " + aLength + '|'); }
	
	void maxedOut(int qPos = -1) {
		bool sendPos = /* BOOLSETTING(ENABLE_REAL_UPLOAD_QUEUE) &&  */
			 !isSet(UserConnection::FLAG_STEALTH) && qPos >= 0;

		if(isSet(FLAG_NMDC)) {
			send(PMAX + (sendPos ? (" " + Util::toString(qPos)) : Util::emptyString) + "|");
		} else {
			AdcCommand cmd(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_SLOTS_FULL, "Slots full");
			if(sendPos) {
				cmd.addParam("QP", Util::toString(qPos));
			}
			send(cmd);
		}
	}
	
	
	void fileNotAvail(const std::string& msg = FILE_NOT_AVAILABLE) { isSet(FLAG_NMDC) ? send("$Error " + msg + "|") : send(AdcCommand(AdcCommand::SEV_RECOVERABLE, AdcCommand::ERROR_FILE_NOT_AVAILABLE, msg)); }
	void getListLen() { send(PGLL + "|"); }

	// ADC Stuff
	void sup(const StringList& features) { 
		AdcCommand c(AdcCommand::CMD_SUP);
		for(StringIterC i = features.begin(); i != features.end(); ++i)
			c.addParam(*i);
		send(c);
	}
	void inf(bool withToken);
	void get(const string& aType, const string& aName, const int64_t aStart, const int64_t aBytes) {  send(AdcCommand(AdcCommand::CMD_GET).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes))); }
	void snd(const string& aType, const string& aName, const int64_t aStart, const int64_t aBytes) {  send(AdcCommand(AdcCommand::CMD_SND).addParam(aType).addParam(aName).addParam(Util::toString(aStart)).addParam(Util::toString(aBytes))); }

	void send(const AdcCommand& c) { send(c.toString(0, isSet(FLAG_NMDC))); }

	void supports(const StringList& feat) { 
		string x;
		for(StringList::const_iterator i = feat.begin(); i != feat.end(); ++i) {
			x+= *i + ' ';
		}
		send(PSUP + " " + x + '|');
	}
	void setDataMode(int64_t aBytes = -1) 
	  {
		  dcassert(socket);
		  if(socket)
		     socket->setDataMode(aBytes); 
	}
	void setLineMode(size_t rollback) 
	{ 
		dcassert(socket); 
		if(socket)
		   socket->setLineMode(rollback); 
	}

	void connect(const string& aServer, uint16_t aPort) throw(SocketException, ThreadException);
	void accept(const Socket& aServer) throw(SocketException, ThreadException);

	void disconnect(bool graceless = false) { if(socket) socket->disconnect(graceless); }
	void transmitFile(InputStream* f) { if(socket) socket->transmitFile(f); }

	const string& getDirectionString() const {
		dcassert(isSet(FLAG_UPLOAD) ^ isSet(FLAG_DOWNLOAD));
		return isSet(FLAG_UPLOAD) ? UPLOAD : DOWNLOAD;
	}

	const UserPtr& getUser() const { return user; }
	UserPtr& getUser() { return user; }
#ifdef PPA_INCLUDE_SSL
	bool isSecure() const { return socket && socket->isSecure(); }
	bool isTrusted() const { return socket && socket->isTrusted(); }
#endif
	string getRemoteIp() const { if(socket) return socket->getIp(); else return Util::emptyString; }
	string getRemoteHost(const string& aIp) const { if(socket) return socket->getRemoteHost(aIp); else return Util::emptyString; }
	Download* getDownload() { dcassert(isSet(FLAG_DOWNLOAD)); return download; }
	void setDownload(Download* d) { dcassert(isSet(FLAG_DOWNLOAD)); download = d; }
	Upload* getUpload() { dcassert(isSet(FLAG_UPLOAD)); return upload; }
	void setUpload(Upload* u) { dcassert(isSet(FLAG_UPLOAD)); upload = u; }

	void reconnect() {
		disconnect();
		Thread::sleep(100);
		ClientManager::getInstance()->connect(user);
	}
	
	void handle(AdcCommand::SUP t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::INF t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::GET t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::SND t, const AdcCommand& c) { fire(t, this, c);	}
	void handle(AdcCommand::STA t, const AdcCommand& c) { fire(t, this, c);	}
	void handle(AdcCommand::RES t, const AdcCommand& c) { fire(t, this, c); }
	void handle(AdcCommand::GFI t, const AdcCommand& c) { fire(t, this, c);	}

	// Ignore any other ADC commands for now
	template<typename T> void handle(T , const AdcCommand& ) { }

	GETSET(string, hubUrl, HubUrl);
	GETSET(string, token, Token);
	GETSET(string, encoding, Encoding);
	GETSET(States, state, State);
	GETSET(uint64_t, lastActivity, LastActivity);

	BufferedSocket const* getSocket() { return socket; } 
	void garbageCommand() { 
		string tmp;
		tmp.reserve(20);
		for(int i = 0; i < 20; i++) {
			tmp.append(1, (char)Util::rand('a', 'z'));
		}
		send("$"+tmp+"|");
	}

private:
	BufferedSocket* socket;
#ifdef PPA_INCLUDE_SSL
	bool secure;
#endif
	UserPtr user;

	static const string UPLOAD, DOWNLOAD;
	
	union {
		Download* download;
		Upload* upload;
	};

	// We only want ConnectionManager to create this...
	UserConnection(bool
#ifdef PPA_INCLUDE_SSL
		secure_
#endif
		) throw() : state(STATE_UNCONNECTED), lastActivity(0), 
		socket(0), 
#ifdef PPA_INCLUDE_SSL
		secure(secure_), 
#endif
		download(NULL) { 
	}

	virtual ~UserConnection() throw() {
		BufferedSocket::putSocket(socket);
	}
	friend struct DeleteFunction;

	UserConnection(const UserConnection&);
	UserConnection& operator=(const UserConnection&);

	void setUser(const UserPtr& aUser);
        void setUploadLimit(FavoriteUser::UPLOAD_LIMIT lim); // !SMT!-S

	void onLine(const string& aLine) throw();
	
	void send(const string& aString) {
		lastActivity = GET_TICK();
		COMMAND_DEBUG(aString, DebugManager::CLIENT_OUT, getRemoteIp());
		if(socket)
		   socket->write(aString);
	}

	virtual void on(Connected) throw() {
        lastActivity = GET_TICK();
        fire(UserConnectionListener::Connected(), this); 
    }
	virtual void on(Line, const string&) throw();
	virtual void on(Data, uint8_t* data, size_t len) throw() { 
        lastActivity = GET_TICK(); 
        fire(UserConnectionListener::Data(), this, data, len); 
    }
	virtual void on(BytesSent, size_t bytes, size_t actual) throw() { 
        lastActivity = GET_TICK();
        fire(UserConnectionListener::BytesSent(), this, bytes, actual); 
    }
	virtual void on(ModeChange) throw() { 
        lastActivity = GET_TICK(); 
        fire(UserConnectionListener::ModeChange(), this); 
    }
	virtual void on(TransmitDone) throw() { fire(UserConnectionListener::TransmitDone(), this); }
	virtual void on(Failed, const string&) throw();
};

#endif // !defined(USER_CONNECTION_H)

/**
 * @file
 * $Id: UserConnection.h,v 1.3 2008/03/31 15:15:31 alexey Exp $
 */
