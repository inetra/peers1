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

#if !defined(SSLSOCKET_H)
#define SSLSOCKET_H

#ifdef PPA_INCLUDE_SSL
#include "Socket.h"
#include "Singleton.h"

#include <openssl/ssl.h>

#ifndef SSL_SUCCESS
#define SSL_SUCCESS 1
#endif

#ifdef YASSL_VERSION
using namespace yaSSL;
#endif

class CryptoManager;

class SSLSocket : public Socket {
public:
	~SSLSocket() throw() {}

	void accept(const Socket& listeningSocket) throw(SocketException);
	void connect(const string& aIp, uint16_t aPort) throw(SocketException);
	int read(void* aBuffer, int aBufLen) throw(SocketException);
	int write(const void* aBuffer, int aLen) throw(SocketException);
	int wait(uint32_t millis, int waitFor) throw(SocketException);
	void shutdown() throw();
	void close() throw();

	bool isSecure() const throw() { return true; }
	bool isTrusted() const throw();

private:
	friend class CryptoManager;

	SSLSocket(SSL_CTX* context) throw(SocketException);
	SSLSocket(const SSLSocket&);
	SSLSocket& operator=(const SSLSocket&);

	SSL_CTX* ctx;
	SSL* ssl;

	int checkSSL(int ret) throw(SocketException);
};
#endif
#endif // SSLSOCKET_H
