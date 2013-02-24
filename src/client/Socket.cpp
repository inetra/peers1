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

#include "Socket.h"

#include "SettingsManager.h"
#include "ResourceManager.h"
#include "TimerManager.h"

#include "LogManager.h"

string Socket::udpServer;
uint16_t Socket::udpPort;

#include "peers/qos2.h"

#define checkconnected() if(!isConnected()) throw SocketException(ENOTCONN))

#ifdef _DEBUG

SocketException::SocketException(int aError) throw() {
	error = "SocketException: " + errorToString(aError);
	dcdebug("Thrown: %s\n", error.c_str());
}

#else // _DEBUG

SocketException::SocketException(int aError) throw() : Exception(errorToString(aError)) { }

#endif

SocketException::SocketException(const string& errorText, int aError) throw(): Exception(errorText + ": " + errorToString(aError)) { }

Socket::Stats Socket::stats = { 0, 0 };

static const uint32_t SOCKS_TIMEOUT = 30000;

string SocketException::errorToString(int aError) throw() {
	string msg = Util::translateError(aError);
	if(msg.empty())
	{
		char tmp[64];
		snprintf(tmp, sizeof(tmp), CSTRING(UNKNOWN_ERROR), aError);
		msg = tmp;
	}
	return msg;
}

void Socket::create(uint8_t aType /* = TYPE_TCP */) throw(SocketException) {
	if(sock != INVALID_SOCKET)
		disconnect();

	switch(aType) {
	case TYPE_TCP:
		sock = checksocket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
		break;
	case TYPE_UDP:
		sock = checksocket(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
		break;
	default:
		dcasserta(0);
	}
	type = aType;
	setBlocking(true);
}

void Socket::accept(const Socket& listeningSocket) throw(SocketException) {
	if(sock != INVALID_SOCKET) {
		disconnect();
	}
	sockaddr_in sock_addr;
	socklen_t sz = sizeof(sock_addr);
    int l_res_accept = -1; 
	do {
		l_res_accept = ::accept(listeningSocket.sock, (sockaddr*)&sock_addr, &sz);
		sock = l_res_accept;
	} while (l_res_accept < 0 && getLastError() == FLY_EINTR);
	check(sock);

	markSocket();

#ifdef _WIN32
	// Make sure we disable any inherited windows message things for this socket.
	::WSAAsyncSelect(sock, NULL, 0, 0);
#endif

	type = TYPE_TCP;

	setIp(inet_ntoa(sock_addr.sin_addr));
	connected = true;
	setBlocking(true);
}


uint16_t Socket::bind(uint16_t aPort, const string& aIp /* = 0.0.0.0 */) throw (SocketException){
	sockaddr_in sock_addr;
		
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(aPort);
	sock_addr.sin_addr.s_addr = inet_addr(aIp.c_str());
	if(::bind(sock, (sockaddr *)&sock_addr, sizeof(sock_addr)) == SOCKET_ERROR) {
		dcdebug("Bind failed, retrying with INADDR_ANY: %s\n", SocketException(getLastError()).getError().c_str());
		sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	    check(::bind(sock, (sockaddr *)&sock_addr, sizeof(sock_addr)));
	}
	socklen_t size = sizeof(sock_addr);
	getsockname(sock, (sockaddr*)&sock_addr, (socklen_t*)&size);
	return ntohs(sock_addr.sin_port);
}

void Socket::listen() throw(SocketException) {
	check(::listen(sock, 20));
	connected = true;
}

void Socket::connect(const string& aAddr, uint16_t aPort) throw(SocketException) {
	sockaddr_in  serv_addr;

	if(sock == INVALID_SOCKET) {
		create(TYPE_TCP);
	}

	vector<string> addresses;
	resolve(aAddr, addresses);
	dcassert(!addresses.empty());

	memzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_port = htons(aPort);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(addresses[0].c_str());

	int result;
	do {
		result = ::connect(sock,(sockaddr*)&serv_addr,sizeof(serv_addr));
	} while (result < 0 && getLastError() == FLY_EINTR);
	check(result, true);

	connected = true;
	setIp(addresses[0]);
	setPort(aPort);

	StringMap params;
	params["message"] = "Connected to " + aAddr + ":" + Util::toString(aPort);
	LOG(LogManager::SYSTEM, params);

	markSocket();
}

namespace {
	inline uint64_t timeLeft(uint64_t start, uint64_t timeout) {
		if(timeout == 0) {
			return 0;
		}			
		uint64_t now = GET_TICK();
		if(start + timeout < now)
			throw SocketException(STRING(CONNECTION_TIMEOUT));
		return start + timeout - now;
	}
}

void Socket::socksConnect(const string& aAddr, uint16_t aPort, uint32_t timeout) throw(SocketException) {

	if(SETTING(SOCKS_SERVER).empty() || SETTING(SOCKS_PORT) == 0) {
		throw SocketException(STRING(SOCKS_FAILED));
	}

	bool oldblock = getBlocking();
	setBlocking(false);

	uint64_t start = GET_TICK();

	connect(SETTING(SOCKS_SERVER), static_cast<uint16_t>(SETTING(SOCKS_PORT)));

	if(wait(timeLeft(start, timeout), WAIT_CONNECT) != WAIT_CONNECT) {
		throw SocketException(STRING(SOCKS_FAILED));
	}

	socksAuth(timeLeft(start, timeout));

	vector<uint8_t> connStr;

	// Authenticated, let's get on with it...
	connStr.push_back(5);			// SOCKSv5
	connStr.push_back(1);			// Connect
	connStr.push_back(0);			// Reserved

	if(BOOLSETTING(SOCKS_RESOLVE)) {
		connStr.push_back(3);		// Address type: domain name
		connStr.push_back((uint8_t)aAddr.size());
		connStr.insert(connStr.end(), aAddr.begin(), aAddr.end());
	} else {
		connStr.push_back(1);		// Address type: IPv4;
		unsigned long addr = inet_addr(resolve(aAddr).c_str());
		uint8_t* paddr = (uint8_t*)&addr;
		connStr.insert(connStr.end(), paddr, paddr+4);
	}

	uint16_t port = htons(aPort);
	uint8_t* pport = (uint8_t*)&port;
	connStr.push_back(pport[0]);
	connStr.push_back(pport[1]);

	writeAll(&connStr[0], connStr.size(), timeLeft(start, timeout));

	// We assume we'll get a ipv4 address back...therefore, 10 bytes...
	/// @todo add support for ipv6
	if(readAll(&connStr[0], 10, timeLeft(start, timeout)) != 10) {
		throw SocketException(STRING(SOCKS_FAILED));
	}

	if(connStr[0] != 5 || connStr[1] != 0) {
		throw SocketException(STRING(SOCKS_FAILED));
	}

	in_addr sock_addr;

	memzero(&sock_addr, sizeof(sock_addr));
	sock_addr.s_addr = *((unsigned long*)&connStr[4]);
	setIp(inet_ntoa(sock_addr));

	if(oldblock)
		setBlocking(oldblock);
}

void Socket::socksAuth(uint64_t timeout) throw(SocketException) {
	vector<uint8_t> connStr;

	uint64_t start = GET_TICK();

	if(SETTING(SOCKS_USER).empty() && SETTING(SOCKS_PASSWORD).empty()) {
		// No username and pw, easier...=)
		connStr.push_back(5);			// SOCKSv5
		connStr.push_back(1);			// 1 method
		connStr.push_back(0);			// Method 0: No auth...

		writeAll(&connStr[0], 3, timeLeft(start, timeout));

		if(readAll(&connStr[0], 2, timeLeft(start, timeout)) != 2) {
			throw SocketException(STRING(SOCKS_FAILED));
		}

		if(connStr[1] != 0) {
			throw SocketException(STRING(SOCKS_NEEDS_AUTH));
		}				
	} else {
		// We try the username and password auth type (no, we don't support gssapi)

		connStr.push_back(5);			// SOCKSv5
		connStr.push_back(1);			// 1 method
		connStr.push_back(2);			// Method 2: Name/Password...
		writeAll(&connStr[0], 3, timeLeft(start, timeout));

		if(readAll(&connStr[0], 2, timeLeft(start, timeout)) != 2) {
			throw SocketException(STRING(SOCKS_FAILED));
		}
		if(connStr[1] != 2) {
			throw SocketException(STRING(SOCKS_AUTH_UNSUPPORTED));
		}

		connStr.clear();
		// Now we send the username / pw...
		connStr.push_back(1);
		connStr.push_back((uint8_t)SETTING(SOCKS_USER).length());
		connStr.insert(connStr.end(), SETTING(SOCKS_USER).begin(), SETTING(SOCKS_USER).end());
		connStr.push_back((uint8_t)SETTING(SOCKS_PASSWORD).length());
		connStr.insert(connStr.end(), SETTING(SOCKS_PASSWORD).begin(), SETTING(SOCKS_PASSWORD).end());

		writeAll(&connStr[0], connStr.size(), timeLeft(start, timeout));

		if(readAll(&connStr[0], 2, timeLeft(start, timeout)) != 2) {
			throw SocketException(STRING(SOCKS_AUTH_FAILED));
		}

		if(connStr[1] != 0) {
			throw SocketException(STRING(SOCKS_AUTH_FAILED));
		}
	}
}

int Socket::getSocketOptInt(int option) const throw(SocketException) {
	int val;
	socklen_t len = sizeof(val);
	check(::getsockopt(sock, SOL_SOCKET, option, (char*)&val, &len));
	return val;
}

void Socket::setSocketOpt(int option, int val) throw(SocketException) {
	int len = sizeof(val);
	check(::setsockopt(sock, SOL_SOCKET, option, (char*)&val, len));
}

int Socket::read(void* aBuffer, int aBufLen) throw(SocketException) {
	markSocket();
	int len = 0;

	dcassert(type == TYPE_TCP || type == TYPE_UDP);
	do {
	if(type == TYPE_TCP) {
			len = ::recv(sock, (char*)aBuffer, aBufLen, 0);
	} else {
			len = ::recvfrom(sock, (char*)aBuffer, aBufLen, 0, NULL, NULL);
	}
	} while (len < 0 && getLastError() == FLY_EINTR);
	check(len, true);

	if(len > 0) {
		stats.totalDown += len;
	}

	return len;
}

int Socket::read(void* aBuffer, int aBufLen, string &aIP) throw(SocketException) {
	dcassert(type == TYPE_UDP);

	markSocket();

	sockaddr_in remote_addr = { 0 };
	socklen_t addr_length = sizeof(remote_addr);

	int len;
	do {
		len = ::recvfrom(sock, (char*)aBuffer, aBufLen, 0, (sockaddr*)&remote_addr, &addr_length);
	} while (len < 0 && getLastError() == FLY_EINTR);

	check(len, true);
	if(len > 0) {
		aIP = inet_ntoa(remote_addr.sin_addr);
		stats.totalDown += len;
	} else {
		aIP.clear();
	}
	return len;
}

int Socket::readAll(void* aBuffer, int aBufLen, uint64_t timeout) throw(SocketException) {
	markSocket();
	uint8_t* buf = (uint8_t*)aBuffer;
	int i = 0;
	while(i < aBufLen) {
		int j = read(buf + i, aBufLen - i);
		if(j == 0) {
			return i;
		} else if(j == -1) {
			if(wait(timeout, WAIT_READ) != WAIT_READ) {
				return i;
			}
			continue;
		}

		i += j;
	}
	return i;
}

void Socket::writeAll(const void* aBuffer, int aLen, uint64_t timeout) throw(SocketException) {
	markSocket();

	const uint8_t* buf = (const uint8_t*)aBuffer;
	int pos = 0;
	// No use sending more than this at a time...
	int sendSize = getSocketOptInt(SO_SNDBUF);

	while(pos < aLen) {
		int i = write(buf+pos, (int)min(aLen-pos, sendSize));
		if(i == -1) {
			wait(timeout, WAIT_WRITE);
		} else {
			pos+=i;
			stats.totalUp += i;
		}
	}
}

int Socket::write(const void* aBuffer, int aLen) throw(SocketException) {
	markSocket();

	int sent;
	do {
		sent = ::send(sock, (const char*)aBuffer, aLen, 0);
	} while (sent < 0 && getLastError() == FLY_EINTR);

	check(sent, true);
	if(sent > 0) {
		stats.totalUp += sent;
	}
	return sent;
}

/**
* Sends data, will block until all data has been sent or an exception occurs
* @param aBuffer Buffer with data
* @param aLen Data length
* @throw SocketExcpetion Send failed.
*/
void Socket::writeTo(const string& aAddr, uint16_t aPort, const void* aBuffer, int aLen, bool proxy) throw(SocketException) {
	markSocket();

	if(aLen <= 0) 
		return;

	uint8_t* buf = (uint8_t*)aBuffer;
	if(sock == INVALID_SOCKET) {
		create(TYPE_UDP);
		setSocketOpt(SO_SNDTIMEO, 250);
	}

	dcassert(type == TYPE_UDP);

	sockaddr_in serv_addr;

	if(aAddr.empty() || aPort == 0) {
		throw SocketException(EADDRNOTAVAIL);
	}

	memzero(&serv_addr, sizeof(serv_addr));

	int sent;
	if(SETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5 && proxy) {
		if(udpServer.empty() || udpPort == 0) {
			throw SocketException(STRING(SOCKS_SETUP_ERROR));
		}

		serv_addr.sin_port = htons(udpPort);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(udpServer.c_str());
		
		string s = BOOLSETTING(SOCKS_RESOLVE) ? resolve(ip) : ip;

		vector<uint8_t> connStr;

		connStr.push_back(0);		// Reserved
		connStr.push_back(0);		// Reserved
		connStr.push_back(0);		// Fragment number, always 0 in our case...
		
		if(BOOLSETTING(SOCKS_RESOLVE)) {
			connStr.push_back(3);
			connStr.push_back((uint8_t)s.size());
			connStr.insert(connStr.end(), aAddr.begin(), aAddr.end());
		} else {
			connStr.push_back(1);		// Address type: IPv4;
			unsigned long addr = inet_addr(resolve(aAddr).c_str());
			uint8_t* paddr = (uint8_t*)&addr;
			connStr.insert(connStr.end(), paddr, paddr+4);
		}

		connStr.insert(connStr.end(), buf, buf + aLen);

		do {
			sent = ::sendto(sock, (const char*)&connStr[0], connStr.size(), 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		} while (sent < 0 && getLastError() == FLY_EINTR);
	} else {
		serv_addr.sin_port = htons(aPort);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(resolve(aAddr).c_str());
		do {
			sent = ::sendto(sock, (const char*)aBuffer, (int)aLen, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
		} while (sent < 0 && getLastError() == FLY_EINTR);
	}

	check(sent);
	stats.totalUp += sent;
}

/**
 * Blocks until timeout is reached one of the specified conditions have been fulfilled
 * @param millis Max milliseconds to block.
 * @param waitFor WAIT_*** flags that set what we're waiting for, set to the combination of flags that
 *				  triggered the wait stop on return (==WAIT_NONE on timeout)
 * @return WAIT_*** ored together of the current state.
 * @throw SocketException Select or the connection attempt failed.
 */
int Socket::wait(uint64_t millis, int waitFor) throw(SocketException) {
	timeval tv;
	fd_set rfd, wfd, efd;
	fd_set *rfdp = NULL, *wfdp = NULL;
	tv.tv_sec = static_cast<uint32_t>(millis/1000);
	tv.tv_usec = static_cast<uint32_t>((millis%1000)*1000); 

	if(waitFor & WAIT_CONNECT) {
		dcassert(!(waitFor & WAIT_READ) && !(waitFor & WAIT_WRITE));

		int result;
		do {
		FD_ZERO(&wfd);
		FD_ZERO(&efd);

		FD_SET(sock, &wfd);
		FD_SET(sock, &efd);
			result = select((int)(sock+1), 0, &wfd, &efd, &tv);
		} while (result < 0 && getLastError() == FLY_EINTR);
		check(result);

		if(FD_ISSET(sock, &wfd)) {
			return WAIT_CONNECT;
		}

		if(FD_ISSET(sock, &efd)) {
			int y = 0;
			socklen_t z = sizeof(y);
			check(getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&y, &z));

			if(y != 0)
				throw SocketException(y);
			// No errors! We're connected (?)...
			return WAIT_CONNECT;
		}
		return 0;
	}

	int result;
	do {
	if(waitFor & WAIT_READ) {
		dcassert(!(waitFor & WAIT_CONNECT));
		rfdp = &rfd;
		FD_ZERO(rfdp);
		FD_SET(sock, rfdp);
	}
	if(waitFor & WAIT_WRITE) {
		dcassert(!(waitFor & WAIT_CONNECT));
		wfdp = &wfd;
		FD_ZERO(wfdp);
		FD_SET(sock, wfdp);
	}

		result = select((int)(sock+1), rfdp, wfdp, NULL, &tv);
	} while (result < 0 && getLastError() == FLY_EINTR);
	check(result);

	waitFor = WAIT_NONE;

	if(rfdp && FD_ISSET(sock, rfdp)) {
		waitFor |= WAIT_READ;
	}
	if(wfdp && FD_ISSET(sock, wfdp)) {
		waitFor |= WAIT_WRITE;
	}

	return waitFor;
}

string Socket::resolve(const string& aDns) {
	sockaddr_in sock_addr;

	memzero(&sock_addr, sizeof(sock_addr));
	sock_addr.sin_port = 0;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_addr.s_addr = inet_addr(aDns.c_str());

	if (sock_addr.sin_addr.s_addr == INADDR_NONE) {   /* server address is a name or invalid */
		hostent* host;
		host = gethostbyname(aDns.c_str());
		if (host == NULL) {
			return Util::emptyString;
		}
		sock_addr.sin_addr.s_addr = *((uint32_t*)host->h_addr);
		return inet_ntoa(sock_addr.sin_addr);
	} else {
		return aDns;
	}
}

void Socket::resolve(const string& aDns, vector<string>& addresses) throw(SocketException) {
	addresses.clear();
	uint32_t addr = inet_addr(aDns.c_str());
	if (addr == INADDR_NONE) {   /* server address is a name or invalid */
		hostent* host = gethostbyname(aDns.c_str());
		if (host == NULL) {
			throw SocketException(STRING(HOST_RESOLVE_FAILED) + aDns, getLastError());
		}
		if ((size_t) host->h_length != sizeof(uint32_t)) {
			throw SocketException("Wrong IP address length " + Util::toString(host->h_length));
		}
		uint32_t** ptr = (uint32_t**) host->h_addr_list;
		while (*ptr != 0) {
			in_addr a;
			a.s_addr = **ptr++;
			const string addr = inet_ntoa(a);
			dcdebug("resolve(%s)=%s\n", aDns.c_str(), addr.c_str());
			addresses.push_back(addr);
		}
		if (addresses.empty()) {
			throw SocketException("No IP addresses!");
		}
	}
	else {
		in_addr a;
		a.s_addr = addr;
		addresses.push_back(inet_ntoa(a));
	}
}

string Socket::getLocalIp() const throw() {
	if(sock == INVALID_SOCKET)
		return Util::emptyString;

	sockaddr_in sock_addr;
	socklen_t len = sizeof(sock_addr);
	if(getsockname(sock, (sockaddr*)&sock_addr, &len) == 0) {
		return inet_ntoa(sock_addr.sin_addr);
	}
	return Util::emptyString;
}

void Socket::socksUpdated() {
	udpServer.clear();
	udpPort = 0;
	
	if(SETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5) {
		try {
			Socket s;
			s.setBlocking(false);
			s.connect(SETTING(SOCKS_SERVER), static_cast<uint16_t>(SETTING(SOCKS_PORT)));
			s.socksAuth(SOCKS_TIMEOUT);

			char connStr[10];
			connStr[0] = 5;			// SOCKSv5
			connStr[1] = 3;			// UDP Associate
			connStr[2] = 0;			// Reserved
			connStr[3] = 1;			// Address type: IPv4;
			*((long*)(&connStr[4])) = 0;		// No specific outgoing UDP address
			*((uint16_t*)(&connStr[8])) = 0;	// No specific port...
			
			s.writeAll(connStr, 10, SOCKS_TIMEOUT);
			
			// We assume we'll get a ipv4 address back...therefore, 10 bytes...if not, things
			// will break, but hey...noone's perfect (and I'm tired...)...
			if(s.readAll(connStr, 10, SOCKS_TIMEOUT) != 10) {
				return;
			}

			if(connStr[0] != 5 || connStr[1] != 0) {
				return;
			}

			udpPort = static_cast<uint16_t>(ntohs(*((uint16_t*)(&connStr[8]))));

			in_addr serv_addr;
			
			memzero(&serv_addr, sizeof(serv_addr));
			serv_addr.s_addr = *((long*)(&connStr[4]));
			udpServer = inet_ntoa(serv_addr);
		} catch(const SocketException&) {
			dcdebug("Socket: Failed to register with socks server\n");
		}
	}
}

void Socket::shutdown() throw() {
	if(sock != INVALID_SOCKET)
                ::shutdown(sock, SD_BOTH); // !DC++!
}

void Socket::close() throw() {
	if(sock != INVALID_SOCKET) {
#ifdef _WIN32
		unmarkSocket();
		::closesocket(sock);
#else
		::close(sock);
#endif
		connected = false;
		sock = INVALID_SOCKET;
	}
}

void Socket::disconnect() throw() {
	shutdown();
	close();
}

const string Socket::getRemoteHost(const string& aIp) {
	if(aIp.empty())
		return Util::emptyString;
	hostent *h = NULL;
	unsigned int addr;
	addr = inet_addr(aIp.c_str());

	h = gethostbyaddr(reinterpret_cast<char *>(&addr), 4, AF_INET);
	if (h == NULL) {
		return Util::emptyString;
	} else {
		return h->h_name;
	}
}

// !SMT!-IP
#ifdef PPA_INCLUDE_DNS
Socket::DNSCache Socket::dnsCache;
string Socket::DNSCache::nslookup(const string &ip)
{
    if (ip.empty()) return ip;

    switch (SETTING(NSLOOKUP_MODE))
    {
        case NSLOOKUP_IMMEDIATE:
        {
            Lock l(nscs);
            DNSIter i = nscache.find(ip);
            if (i != nscache.end()) return i->second;
            return nscache[ip] = Socket::getRemoteHost(ip);
        }
        case NSLOOKUP_DELAYED:
        {
            Lock l(nscs);
            DNSIter i = nscache.find(ip);
            string res;
            if (i != nscache.end()) return i->second;
            if (!started) { started = true; start(); }
            DNSQIter j;
            for (j = nsqueue.begin(); j != nsqueue.end() && *j != ip; j++);
            if (j == nsqueue.end()) nsqueue.push_back(ip);
            return Util::emptyString;
        }
        default: // case NSLOOKUP_DISABLED:
            return Util::emptyString;
    }
}
int Socket::DNSCache::run()
{
    bool empty;
    string ip, domain;
    for (;;) {
        {
            Lock l(nscs);
            empty = nsqueue.empty();
        }
        if (empty) {
            if (needShutdown.wait(10 + SETTING(NSLOOKUP_DELAY))) break;
            continue;
        }
        {
            Lock l(nscs);
            ip = nsqueue.front();
        }
        domain = Socket::getRemoteHost(ip);
        if (domain.empty()) domain = ip;
        {
            Lock l(nscs);
            nsqueue.pop_front();
            nscache[ip] = domain;
        }
        if (needShutdown.wait(SETTING(NSLOOKUP_DELAY))) break;
    }
    shutdownComplete.signal();
    return 0;
}
// end !SMT!-IP
#endif

void Socket::initQoS()
{
#ifdef _WIN32
	mQOSCreateHandle = NULL;
	mQOSCloseHandle = NULL;
	mQOSAddSocketToFlow = NULL;
	mQOSRemoveSocketFromFlow = NULL;
	mQOSSetFlow = NULL;

	// detect qWAVE available
	qWAVEhandle = LoadLibraryA("qwave.dll");
	if (qWAVEhandle != NULL) {
		// Windows >= Vista
		mQOSCreateHandle = (pfnQOSCreateHandle) GetProcAddress(qWAVEhandle, "QOSCreateHandle");
		mQOSCloseHandle = (pfnQOSCloseHandle) GetProcAddress(qWAVEhandle, "QOSCloseHandle");
		mQOSAddSocketToFlow = (pfnQOSAddSocketToFlow) GetProcAddress(qWAVEhandle, "QOSAddSocketToFlow");
		mQOSRemoveSocketFromFlow = (pfnQOSRemoveSocketFromFlow) GetProcAddress(qWAVEhandle, "QOSRemoveSocketFromFlow");
		mQOSSetFlow = (pfnQOSSetFlow) GetProcAddress(qWAVEhandle, "QOSSetFlow");

		if (mQOSCreateHandle == NULL || mQOSCloseHandle == NULL || mQOSAddSocketToFlow == NULL || mQOSRemoveSocketFromFlow == NULL || mQOSSetFlow == NULL) {
			// bad qWAVE?
			FreeLibrary(qWAVEhandle);
			qWAVEhandle = NULL;
		} else {
			QOSversion.MajorVersion = 1;
			QOSversion.MinorVersion = 0;
			if (!mQOSCreateHandle(&QOSversion, &hQoS)) {
				FreeLibrary(qWAVEhandle);
				qWAVEhandle = NULL;
			}
		}
		
		QoSFlowId = 0;
	}
#endif
	marked = false;
	DSCP = SETTING(DEFAULT_DSCP_MARK);
}

void Socket::finishQoS()
{
#ifdef _WIN32
	if (qWAVEhandle != NULL) {
		mQOSCloseHandle(hQoS);
		hQoS = NULL;
		FreeLibrary(qWAVEhandle);
		qWAVEhandle = NULL;
	}
#endif
}

void Socket::setDSCP(char newValue)
{
	unmarkSocket();
	DSCP = newValue;
	markSocket();
}


void Socket::markSocket()
{
	if (marked) return;

	unmarkSocket();
#ifdef _WIN32
	if (qWAVEhandle != NULL) {
		if (mQOSAddSocketToFlow(hQoS, sock, 0, QOSTrafficTypeBestEffort, QOS_NON_ADAPTIVE_FLOW, &QoSFlowId)) {
			DWORD dscp = DSCP;
			mQOSSetFlow(hQoS, QoSFlowId, QOSSetOutgoingDSCPValue, sizeof(dscp), &dscp, 0, NULL);
			marked = true;
		} else {
			marked = false;
		}
	} else {
#endif
		// This works on Windows < Vista with 
		// DisableUserTOSSetting set to 0 in registry
		// see http://support.microsoft.com/kb/248611
		const char ipTOS = DSCP << 2;
		setsockopt(sock, IPPROTO_IP, IP_TOS, &ipTOS, sizeof(ipTOS));
		marked = true;
#ifdef _WIN32
	}
#endif
}

void Socket::unmarkSocket()
{
	if (!marked) return;

	marked = false;
#ifdef _WIN32
	if (qWAVEhandle != NULL && QoSFlowId != 0) {
		mQOSRemoveSocketFromFlow(hQoS, sock, QoSFlowId, 0);
		QoSFlowId = 0;
	} else {
		const char ipTOS = 0;
		setsockopt(sock, IPPROTO_IP, IP_TOS, &ipTOS, sizeof(ipTOS));
	}
#endif
}

/**
 * @file
 * $Id: Socket.cpp,v 1.1.2.1 2008/06/15 06:41:45 alexey Exp $
 */
