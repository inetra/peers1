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

#include "BufferedSocket.h"

#include "ResourceManager.h"
//[-]PPA [Doxygen 1.5.1]  #include "TimerManager.h"
#include "SettingsManager.h"

#include "Streams.h"
//[-]PPA [Doxygen 1.5.1] #include "SSLSocket.h"
#include "CryptoManager.h"

#include "UploadManager.h"
#include "DownloadManager.h"

#include <iostream>

// Polling is used for tasks...should be fixed...
#define POLL_TIMEOUT 250

class EOFSocketException : public SocketException {
public:
	EOFSocketException(): SocketException("This socket has been closed...") {}
};

BufferedSocket::BufferedSocket(char aSeparator) throw() : 
separator(aSeparator), mode(MODE_LINE), filterIn(NULL), sleep(0), // !SMT!-S
dataBytes(0), rollback(0), failed(false), sock(0), disconnecting(false), superUser(false)
{
	Thread::safeInc(sockets);
}

volatile long BufferedSocket::sockets = 0;

BufferedSocket::~BufferedSocket() throw() {
	delete sock;
	delete filterIn;
	Thread::safeDec(sockets);
}

void BufferedSocket::setMode (Modes aMode, size_t aRollback) {
	if (mode == aMode) {
		dcdebug ("WARNING: Re-entering mode %d\n", mode);
		return;
	}

	if (mode == MODE_ZPIPE) {
		// should not happen!
		if (filterIn) {
			delete filterIn;
			filterIn = NULL;
		}
	}

	mode = aMode;
	switch (aMode) {
		case MODE_LINE:
			rollback = aRollback;
			break;
		case MODE_ZPIPE:
			filterIn = new UnZFilter;
			break;
		case MODE_DATA:
			break;
	}
}

#ifdef PPA_INCLUDE_SSL
void BufferedSocket::accept(const Socket& srv, bool secure, bool allowUntrusted) throw(SocketException, ThreadException) {
#else
void BufferedSocket::accept(const Socket& srv, bool , bool ) throw(SocketException, ThreadException) {
#endif
	dcassert(!sock);
	try {
#ifdef PPA_INCLUDE_SSL
		dcdebug("BufferedSocket::accept() %p\n", (void*)this);
		sock = secure ? CryptoManager::getInstance()->getServerSocket(allowUntrusted) : new Socket;
#else
		sock =  new Socket;
#endif

		sock->accept(srv);
		if(SETTING(SOCKET_IN_BUFFER) > 1023)
			sock->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
    	if(SETTING(SOCKET_OUT_BUFFER) > 1023)
			sock->setSocketOpt(SO_SNDBUF, SETTING(SOCKET_OUT_BUFFER));
		sock->setBlocking(false);

		inbuf.resize(sock->getSocketOptInt(SO_RCVBUF));

		// This lock prevents the shutdown task from being added and executed before we're done initializing the socket
		Lock l(cs);
		start();
		addTask(ACCEPTED, 0);
	} catch(...) {
		delete sock;
		sock = 0;
		throw;
	}
}

#ifdef PPA_INCLUDE_SSL
void BufferedSocket::connect(const string& aAddress, uint16_t aPort, bool secure, bool allowUntrusted, bool proxy) throw(SocketException, ThreadException) {
#else
void BufferedSocket::connect(const string& aAddress, uint16_t aPort, bool, bool , bool proxy) throw(SocketException, ThreadException) {
#endif
	dcassert(!sock);

	try {
#ifdef PPA_INCLUDE_SSL
		dcdebug("BufferedSocket::connect() %p\n", (void*)this);
		sock = secure ? CryptoManager::getInstance()->getClientSocket(allowUntrusted) : new Socket;
#else
		sock = new Socket;
#endif

		sock->create();
		if(SETTING(SOCKET_IN_BUFFER) >= 1024)
			sock->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
		if(SETTING(SOCKET_OUT_BUFFER) >= 1024)
			sock->setSocketOpt(SO_SNDBUF, SETTING(SOCKET_OUT_BUFFER));
		sock->setBlocking(false);

		inbuf.resize(sock->getSocketOptInt(SO_RCVBUF));

		Lock l(cs);
		start();
		addTask(CONNECT, new ConnectInfo(aAddress, aPort, proxy && (SETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5)));
	} catch(...) {
		delete sock;
		sock = 0;
		throw;
	}

}

#define CONNECT_TIMEOUT 30000
void BufferedSocket::threadConnect(const string& aAddr, uint16_t aPort, bool proxy) throw(SocketException) {
	using namespace std;
	dcdebug("threadConnect %s:%d\n", aAddr.c_str(), (int)aPort);
	dcassert(sock);
	if(!sock)
		return;
	fire(BufferedSocketListener::Connecting());

	vector<string> addresses;
	sock->resolve(aAddr, addresses);
	dcassert(!addresses.empty());
	for (size_t i = 0; i < addresses.size(); i++)
	{
		bool retry = false;
		uint64_t startTime = GET_TICK();
	
		if(proxy) {
			sock->socksConnect(addresses[i], aPort, CONNECT_TIMEOUT);
		} else {
			sock->connect(addresses[i], aPort);
		}

		try
		{
			while(sock->wait(POLL_TIMEOUT, Socket::WAIT_CONNECT) != Socket::WAIT_CONNECT) {
				if(disconnecting)
					return;

				if((startTime + CONNECT_TIMEOUT) < GET_TICK()) {
					if ( i == addresses.size() - 1) {
						throw SocketException(STRING(CONNECTION_TIMEOUT));
					}
					else {
						retry = true;
						break;
					}
				}
			}
		}
		catch (SocketException& e)
		{
			if ( i == addresses.size() - 1) {
				throw e;
			}
			else {
				retry = true;
			}
		}
		if (!retry) break;
	}

	fire(BufferedSocketListener::Connected());
}	

void BufferedSocket::threadRead() throw(SocketException) {
	dcassert(sock);
	if(!sock)
		return;
	DownloadManager *dm = DownloadManager::getInstance();
	size_t readsize = inbuf.size();
	bool throttling = false;
	if(mode == MODE_DATA)
	{
		uint32_t getMaximum;
        throttling = dm && dm->throttle() && getSuperUser() == false;
		if (throttling)
		{
			getMaximum = dm->throttleGetSlice();
			readsize = (uint32_t)min((int64_t)inbuf.size(), (int64_t)getMaximum);
			if (readsize <= 0  || readsize > inbuf.size()) { // FIX
				Thread::sleep(dm->throttleCycleTime());
				return;
			}
		}
	}
	int left = sock->read(&inbuf[0], (int)readsize);
	if(left == -1) {
	// EWOULDBLOCK, no data received...
		return;
	} else if(left == 0) {
		// This socket has been closed...
		throw EOFSocketException();
	}
	int bufpos = 0;
	const int total = left;

	while (left > 0) {
		switch (mode) {
			case MODE_ZPIPE:
				if (filterIn != NULL){
				    const int BufSize = 1024;
					AutoArray<uint8_t> buffer (BufSize);
					string l = line;
					// decompress all input data and store in l.
					while (left) {
						size_t in = BufSize;
						size_t used = left;
						bool ret = (*filterIn) ((void *)(&inbuf[0] + total - left), used, &buffer[0], in);
						left -= used;
						l.append ((const char *)&buffer[0], in);
						// if the stream ends before the data runs out, keep remainder of data in inbuf
						if (!ret) {
							bufpos = total-left;
							setMode (MODE_LINE, rollback);
							break;
						}
					}
					// process all lines
					string::size_type pos;
					while ((pos = l.find(separator)) != string::npos) {
                       	if(pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
							fire(BufferedSocketListener::Line(), l.substr(0, pos));
						l.erase (0, pos + 1 /* seperator char */);
					}
					// store remainder
					line = l;

					break;
				}
			case MODE_LINE:
				// Special to autodetect nmdc connections...
				if(separator == 0) {
					if(inbuf[0] == '$') {
						separator = '|';
					} else {
						separator = '\n';
					}
				}
				{
#if 0
					string l = line + string((char*)&inbuf[bufpos], left);
					size_t linePos = 0;
					string::size_type pos;
					while ((pos = l.find(separator, linePos)) != string::npos) {
						if(pos > linePos) // check empty (only pipe) command and don't waste cpu with it ;o)
							fire(BufferedSocketListener::Line(), l.substr(linePos, pos - linePos));
						left -= (pos - linePos + 1);
						if (mode != MODE_LINE) {
							// we changed mode; remainder of line is invalid.
							line.clear();
							bufpos = total - left;
							break;
						}
						linePos = pos + 1;
					}
					if (pos == string::npos) {
						// no more separators; keep remainder of line
						left = 0;
						line = l.substr(linePos);
					}
#else
					string l = line + string ((char*)&inbuf[bufpos], left);
					string::size_type pos;
					while ((pos = l.find(separator)) != string::npos) {
						if(pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
							fire(BufferedSocketListener::Line(), l.substr(0, pos));
						l.erase (0, pos + 1 /* separator char */);
						if (l.length() < (size_t)left) left = l.length();
						if (mode != MODE_LINE) {
							// we changed mode; remainder of l is invalid.
							l.clear();
							bufpos = total - left;
							break;
						}
					}
					if (pos == string::npos) 
						left = 0;
					line = l;
#endif
				}
				break;
			case MODE_DATA:
				while(left > 0) {
					if(dataBytes == -1) {
						fire(BufferedSocketListener::Data(), &inbuf[bufpos], left);
						bufpos += (left - rollback);
						left = rollback;
						rollback = 0;
					} else {
						int high = (int)min(dataBytes, (int64_t)left);
						fire(BufferedSocketListener::Data(), &inbuf[bufpos], high);
						bufpos += high;
						left -= high;

						dataBytes -= high;
						if(dataBytes == 0) {
							mode = MODE_LINE;
							fire(BufferedSocketListener::ModeChange());
						}
					}
					if (throttling) {
						if (left > 0 && left < (int)readsize) {
							dm->throttleReturnBytes(left - readsize);
						}
						Thread::sleep(dm->throttleCycleTime());
					}
				}
				break;
		}
	}
	
	if(mode == MODE_LINE && line.size() > 16777216) {
		throw SocketException(STRING(COMMAND_TOO_LONG));
	}	
}

void BufferedSocket::threadSendFile(InputStream* file) throw(Exception) {
	dcassert(sock);
	if(!sock)
		return;
	dcassert(file != NULL);
	size_t sockSize = (size_t)sock->getSocketOptInt(SO_SNDBUF);
	size_t bufSize = max(sockSize, (size_t)64*1024);

	vector<uint8_t> readBuf(bufSize);
	vector<uint8_t> writeBuf(bufSize);

	size_t readPos = 0;

	bool readDone = false;
	dcdebug("Starting threadSend\n");
	UploadManager *um = UploadManager::getInstance();
	size_t sendMaximum;
	uint64_t start = 0;
	bool throttling;
	while(true) {
		if(disconnecting)
			return;
                throttling = (BOOLSETTING(THROTTLE_ENABLE) && getSuperUser() == false);
                int UserSleep = getSleep(); // !SMT!-S
		if(!readDone && readBuf.size() > readPos) {
			// Fill read buffer
			size_t bytesRead = readBuf.size() - readPos;

                        // !SMT!-S
                        if (UserSleep >= 0) { // throttleGetSlice() should not be called for superusers, so their downloaded bytes don't count in mUploadLimit
			if(throttling) {
				start = GET_TICK();
				sendMaximum = um->throttleGetSlice();
				if (sendMaximum == (size_t)-1) {
					throttling = false;
					sendMaximum = bytesRead;
				}
				bytesRead = min(bytesRead, sendMaximum);
			}
                           if (UserSleep) { ::Sleep(UserSleep); bytesRead = min(bytesRead, (size_t)1024); } // !SMT!-S
                        }

			size_t actual = file->read(&readBuf[readPos], bytesRead);

			if(bytesRead > 0) {
				fire(BufferedSocketListener::BytesSent(), bytesRead, 0);
			}

			if(actual == 0) {
				readDone = true;
			} else {
				readPos += actual;
			}
		}

		if(readDone && readPos == 0) {
			fire(BufferedSocketListener::TransmitDone());
			return;
		}

		readBuf.swap(writeBuf);
		readBuf.resize(bufSize);
		writeBuf.resize(readPos);
		readPos = 0;

		size_t writePos = 0;

		while(writePos < writeBuf.size()) {
			if(disconnecting)
				return;
			size_t writeSize = min(sockSize / 2, writeBuf.size() - writePos);
			int written = sock->write(&writeBuf[writePos], writeSize);
			if(written > 0) {
				writePos += written;

				fire(BufferedSocketListener::BytesSent(), 0, written);
			} else if(written == -1) {
                                if(!readDone && readPos < readBuf.size() && UserSleep <= 0) { // !SMT!-S
					// Read a little since we're blocking anyway...
					size_t bytesRead = min(readBuf.size() - readPos, readBuf.size() / 2);
					if(throttling) {
						start = GET_TICK();
						sendMaximum = um->throttleGetSlice();
						if (sendMaximum == (size_t)-1)  
						{
							throttling = false;
							sendMaximum = bytesRead;
						}
						bytesRead = (uint32_t)min((int64_t)bytesRead, (int64_t)sendMaximum);
					}
					size_t actual = file->read(&readBuf[readPos], bytesRead);

					if(bytesRead > 0) {
						fire(BufferedSocketListener::BytesSent(), bytesRead, 0);
					}

					if(actual == 0) {
						readDone = true;
					} else {
						readPos += actual;
					}
				} else {
					while(!disconnecting) {
						int w = sock->wait(POLL_TIMEOUT, Socket::WAIT_WRITE | Socket::WAIT_READ);
						if(w & Socket::WAIT_READ) {
							threadRead();
						}
						if(w & Socket::WAIT_WRITE) {
							break;
						}
					}
				}	
			}
			if(throttling) {
				uint32_t cycle_time = um->throttleCycleTime();
				uint64_t sleep_time = cycle_time - (GET_TICK() - start);
				if (sleep_time > 0 && sleep_time <= cycle_time) {
					Thread::sleep(sleep_time);
				}
			}
		}
		Thread::yield();
	}
}

void BufferedSocket::write(const char* aBuf, size_t aLen) throw() {
	dcassert(sock);
	if(!sock)
		return;
	Lock l(cs);
	if(writeBuf.empty())
		addTask(SEND_DATA, 0);

	writeBuf.insert(writeBuf.end(), aBuf, aBuf+aLen);
}

void BufferedSocket::threadSendData() {
	dcassert(sock);
	if(!sock)
		return;
	{
		Lock l(cs);
		if(writeBuf.empty())
			return;

		writeBuf.swap(sendBuf);
	}

	size_t left = sendBuf.size();
	size_t done = 0;
	while(left > 0) {
		if(disconnecting) {
			return;
		}

		int w = sock->wait(POLL_TIMEOUT, Socket::WAIT_READ | Socket::WAIT_WRITE);

		if(w & Socket::WAIT_READ) {
			threadRead();
		}

		if(w & Socket::WAIT_WRITE) {
			int n = sock->write(&sendBuf[done], left);
			if(n > 0) {
				left -= n;
				done += n;
			}
		}
	}
	sendBuf.clear();
}

bool BufferedSocket::checkEvents() {
	while(isConnected() ? taskSem.wait(0) : taskSem.wait()) {
		pair<Tasks, TaskData*> p;
		p.second = 0; //[+]PPA
		{
			Lock l(cs);
			dcassert(m_tasks.size() > 0);
			if(m_tasks.size() == 0)  return false; //[+]PPA	 
			p = m_tasks.front();
			m_tasks.pop_front(); //[+]PPA 
			//[-]PPA m_tasks.erase(m_tasks.begin());
		}
		try {
			if(failed && p.first != SHUTDOWN) {
				dcdebug("BufferedSocket: New command when already failed: %d\n", p.first);
				fail(STRING(DISCONNECTED));
				delete p.second;
				continue;
			}

			switch(p.first) {
				case SEND_DATA:
					threadSendData(); break;
				case SEND_FILE:
					threadSendFile(((SendFileInfo*)p.second)->stream); break;
				case CONNECT: 
					{
						ConnectInfo* ci = (ConnectInfo*)p.second;
						threadConnect(ci->addr, ci->port, ci->proxy); 
						break;
					}
				case DISCONNECT:  
					if(isConnected())
						fail(STRING(DISCONNECTED)); 
					break;
				case SHUTDOWN:
					return false;
				case ACCEPTED:
					break;
			}

			delete p.second;
		} catch(const Exception& e) {
			delete p.second;
			fail(e.getError());
		}
	}
	return true;
}

void BufferedSocket::checkSocket() {
	dcassert(sock);
	if(!sock)
		return;

	int waitFor = sock->wait(POLL_TIMEOUT, Socket::WAIT_READ);

	if(waitFor & Socket::WAIT_READ) {
		threadRead();
	}
}

/**
 * Main task dispatcher for the buffered socket abstraction.
 * @todo Fix the polling...
 */
int BufferedSocket::run() {
	dcdebug("BufferedSocket::run() start %p\n", (void*)this);
	while(true) {
		try {
			if(!checkEvents())
				break;
			checkSocket();
		} catch(const EOFSocketException& e) {
			fail(e.getError(), true);
		} catch(const Exception& e) {
			fail(e.getError());
		}
	}
	dcdebug("BufferedSocket::run() end %p\n", (void*)this);
	delete this;
	return 0;
}

void BufferedSocket::fail(const string& aError, bool dataEnd) {
	if(sock) {
		sock->disconnect();
	}
	if(!failed) {
		failed = true;
		if (dataEnd) {
			fire(BufferedSocketListener::DataEnd(), aError);
		}
		else {
			fire(BufferedSocketListener::Failed(), aError);
		}
	}
}

void BufferedSocket::shutdown() { 
	if(sock) {
		Lock l(cs); 
		disconnecting = true; 
		addTask(SHUTDOWN, 0); 
	} else {
		// Socket thread not running yet, disconnect...
		delete this;
	}
}

/**
 * @file
 * $Id: BufferedSocket.cpp,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
