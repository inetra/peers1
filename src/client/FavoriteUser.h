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

#if !defined(FAVORITE_USER_H)
#define FAVORITE_USER_H


#include "FastAlloc.h"
#include "User.h"
//[-]PPA #include "CID.h"

class FavoriteUser : public Flags {
public:
// !SMT!-S    
	// FavoriteUser(const UserPtr& user_, const string& nick_, const string& hubUrl_) : user(user_), nick(nick_), url(hubUrl_), lastSeen(0) { }

	enum Flags {
                FLAG_GRANTSLOT = 1 << 0,
                FLAG_SUPERUSER = 1 << 1,
                FLAG_IGNOREPRIVATE      = 1 << 2,  // !SMT!-S
                FLAG_FREE_PM_ACCESS     = 1 << 3  // !SMT!-S
	};

        // !SMT!-S
        enum UPLOAD_LIMIT {
           UL_SU  = -2,
           UL_BAN = -1,
           UL_NONE = 0,
           UL_2    = 1,
           UL_5    = 2,
           UL_8    = 3,
           UL_12   = 4,
           UL_16   = 5,
           UL_24   = 6,
           UL_32   = 7,
           UL_64   = 8,
           UL_128  = 9,
           UL_256  = 10
        };

        // !SMT!-S
        static const string GetLimitText(UPLOAD_LIMIT lim)
        {
           switch (lim) {
              case UL_SU:  return ("SU");
              case UL_BAN: return ("BAN");
              case UL_2:   return ("2 Kb/s");
              case UL_5:   return ("5 Kb/s");
              case UL_8:   return ("8 Kb/s");
              case UL_12:  return ("12 Kb/s");
              case UL_16:  return ("16 Kb/s");
              case UL_24:  return ("24 Kb/s");
              case UL_32:  return ("32 Kb/s");
              case UL_64:  return ("64 Kb/s");
              case UL_128: return ("128 Kb/s");
              case UL_256: return ("256 Kb/s");
           }
           return Util::emptyString;
        }

	UserPtr& getUser() { return user; }

        void update(const UserPtr& info); // !SMT!-fix

	GETSET(UserPtr, user, User);
	GETSET(string, nick, Nick);
	GETSET(string, url, Url);
	GETSET(time_t, lastSeen, LastSeen);
	GETSET(string, description, Description);
        GETSET(UPLOAD_LIMIT, uploadLimit, UploadLimit); // !SMT!-S

        // !SMT!-S
        FavoriteUser(const UserPtr& user_, const string& nick_, const string& hubUrl_, UPLOAD_LIMIT limit_ = UL_NONE) : user(user_), nick(nick_), url(hubUrl_), uploadLimit(limit_), lastSeen(0) { }
};

#endif // !defined(FAVORITE_USER_H)

/**
 * @file
 * $Id: FavoriteUser.h,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
