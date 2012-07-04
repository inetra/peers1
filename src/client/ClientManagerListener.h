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

#ifndef DCPLUSPLUS_CLIENT_CLIENT_MANAGER_LISTENER_H
#define DCPLUSPLUS_CLIENT_CLIENT_MANAGER_LISTENER_H


#include "User.h" //{+]PPA

class ClientManagerListener {
public:
	virtual ~ClientManagerListener() { }
	template<int I>	struct X { enum { TYPE = I };  };
	
	typedef X<0> UserConnected;
	typedef X<1> UserUpdated;
	typedef X<2> UserDisconnected;
	typedef X<3> IncomingSearch;
	typedef X<4> ClientConnected;
	typedef X<5> ClientUpdated;
	typedef X<6> ClientDisconnected;

        typedef enum { SEARCH_MISS = 0, SEARCH_PARTIAL_HIT, SEARCH_HIT } SearchReply; // !SMT!-S

	/** User online in at least one hub */
	virtual void on(UserConnected, const UserPtr&) throw() { }
        virtual void on(UserUpdated, const UserPtr&) throw() { }
	/** User offline in all hubs */
	virtual void on(UserDisconnected, const UserPtr&) throw() { }
        virtual void on(IncomingSearch, const string&, const string&, SearchReply) throw() { } // !SMT!-S
	virtual void on(ClientConnected, Client*) throw() { }
	virtual void on(ClientUpdated, Client*) throw() { }
	virtual void on(ClientDisconnected, Client*) throw() { }
};

#endif // !defined(CLIENT_MANAGER_LISTENER_H)

/**
 * @file
 * $Id: ClientManagerListener.h,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
