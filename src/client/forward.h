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

#ifndef DCPLUSPLUS_CLIENT_FORWARD_H_
#define DCPLUSPLUS_CLIENT_FORWARD_H_

/** @file
 * This file contains forward declarations for the various DC++ classes
 */

#include "Pointer.h"

class AdcCommand;

class CID;

class Client;

class ClientManager;

class FavoriteHubEntry;

class RecentHubEntry;

class FavoriteUser;

class Identity;

class OnlineUser;

class QueueItem;

class User;
typedef Pointer<User> UserPtr;
typedef std::vector<UserPtr> UserList;



#endif /*DCPLUSPLUS_CLIENT_FORWARD_H_*/
