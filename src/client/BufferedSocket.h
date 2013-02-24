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

#ifndef DCPLUSPLUS_CLIENT_BUFFERED_SOCKET_H
#define DCPLUSPLUS_CLIENT_BUFFERED_SOCKET_H

#include "Semaphore.h"
//[-]PPA [Doxygen 1.5.1] #include "Thread.h"
#include "Speaker.h"
//[-]PPA [Doxygen 1.5.1] #include "Util.h"
#include "ZUtils.h"
#include "Socket.h"

class InputStream;
class Socket;
class SocketException;

class BufferedSocketListener {
public:
	virtual ~BufferedSocketListener() { }
	template<int I>	struct X { enum { TYPE = I };  };

	typedef X<0> Connecting;
	typedef X<1> Connected;
	typedef X<2> Line;
	typedef X<3> Data;
	typedef X<4> BytesSent;
	typedef X<5> ModeChange;
	typedef X<6> TransmitDone;
	typedef X<7> Failed;
	typedef X<8> DataEnd;

	virtual void on(Connecting) throw() { }
	virtual void on(Connected) throw() { }
	virtual void on(Line, const string&) throw() { }
	virtual void on(Data, uint8_t*, size_t) throw() { }
	virtual void on(BytesSent, size_t, size_t) throw() { }
	virtual void on(ModeChange) throw() { }
	virtual void on(TransmitDone) throw() { }
	virtual void on(Failed, const string&) throw() { }
	virtual void on(DataEnd, const string& msg) throw() { on(Failed(),msg); }
};

class BufferedSocket : public Speaker<BufferedSocketListener>, public Thread
{
public:
	enum Modes {
		MODE_LINE,
		MODE_ZPIPE,
		MODE_DATA
	};

	/**
	 * BufferedSocket factory, each BufferedSocket may only be used to create one connection
	 * @param sep Line separator
	 * @return An unconnected socket
	 */
	static BufferedSocket* getSocket(char sep) throw() { 
		return new BufferedSocket(sep); 
	}

	static void putSocket(BufferedSocket* aSock) { 
		aSock->removeListeners(); 
		aSock->shutdown();
	}

	static void waitShutdown() {
		while(sockets)
			Thread::sleep(100);
	}

	void setDSCP(char newValue);

	void accept(const Socket& srv, bool secure, bool allowUntrusted) throw(SocketException, ThreadException);
	void connect(const string& aAddress, uint16_t aPort, bool secure, bool allowUntrusted, bool proxy) throw(SocketException, ThreadException);

	/** Sets data mode for aBytes bytes. Must be called within onLine. */
	void setDataMode(int64_t aBytes = -1) { mode = MODE_DATA; dataBytes = aBytes; }
	/** 
	 * Rollback is an ugly hack to solve problems with compressed transfers where not all data received
	 * should be treated as data.
	 * Must be called from within onData. 
	 */
	void setLineMode(size_t aRollback) { setMode (MODE_LINE, aRollback);}
	void setMode(Modes mode, size_t aRollback = 0);
	Modes getMode() const { return mode; }
	const string& getIp() const { return sock ? sock->getIp() : Util::emptyString; }
	const string getRemoteHost(const string& aIp) const { return sock ? sock->getRemoteHost(aIp) : Util::emptyString; }
	bool isConnected() const { return sock && sock->isConnected(); }
	
#ifdef PPA_INCLUDE_SSL
	bool isSecure() const { return sock && sock->isSecure(); }
	bool isTrusted() const { return sock && sock->isTrusted(); }
#endif
	void write(const string& aData) throw() { write(aData.data(), aData.length()); }
	void write(const char* aBuf, size_t aLen) throw();
	/** Send the file f over this socket. */
	void transmitFile(InputStream* f) throw() { Lock l(cs); addTask(SEND_FILE, new SendFileInfo(f)); }

	void disconnect(bool graceless = false) throw() 
	  {
		   Lock l(cs); 
		     if(graceless) 
				  disconnecting = true; 
			 addTask(DISCONNECT, 0); 
	  }

	string getLocalIp() const { return sock ? sock->getLocalIp() : Util::getLocalIp(); }

        GETSET(int, sleep, Sleep); // !SMT!-S
        GETSET(char, separator, Separator);
        GETSET(bool, superUser, SuperUser);
private:
	enum Tasks {
		CONNECT,
		DISCONNECT,
		SEND_DATA,
		SEND_FILE,
		SHUTDOWN,
		ACCEPTED
	};

	struct TaskData { 
		virtual ~TaskData() throw() { }
	};
	struct ConnectInfo : public TaskData {
		ConnectInfo(string addr_, uint16_t port_, bool proxy_) : addr(addr_), port(port_), proxy(proxy_) { }
		string addr;
		uint16_t port;
		bool proxy;
	};
	struct SendFileInfo : public TaskData {
		SendFileInfo(InputStream* stream_) : stream(stream_) { }
		InputStream* stream;
	};

	BufferedSocket(char aSeparator) throw();

	// Dummy...
	BufferedSocket(const BufferedSocket&);
	BufferedSocket& operator=(const BufferedSocket&);

	~BufferedSocket() throw();

	CriticalSection cs;

	Semaphore taskSem;
	list<pair<Tasks, TaskData*> > m_tasks;

	Modes mode;
	UnZFilter *filterIn;
	int64_t dataBytes;
	size_t rollback;
	bool failed;
	string line;
	vector<uint8_t> inbuf;
	vector<uint8_t> writeBuf;
	vector<uint8_t> sendBuf;

	Socket* sock;
	bool disconnecting;
	char DSCP;

	int run();

	void threadConnect(const string& aAddr, uint16_t aPort, bool proxy) throw(SocketException);
	void threadRead() throw(SocketException);
	void threadSendFile(InputStream* is) throw(Exception);
	void threadSendData();
	void threadDisconnect();

	void fail(const string& aError, bool dataEnd = false);
	static volatile long sockets;

	bool checkEvents();
	void checkSocket();

	void shutdown();
	void addTask(Tasks task, TaskData* data) 
	{
	     m_tasks.push_back(make_pair(task, data)); 
		 taskSem.signal(); 
	}
};

#endif // !defined(BUFFERED_SOCKET_H)

/**
 * @file
 * $Id: BufferedSocket.h,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
