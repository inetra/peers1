/* 
 * 
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

#ifndef USERINFO_H
#define USERINFO_H

#include "TypedListViewCtrl.h"
#include "UserInfoBase.h"

#include "../client/FastAlloc.h"
#include "../client/TaskQueue.h"

enum {
  COLUMN_FIRST,
  COLUMN_NICK = COLUMN_FIRST, 
  COLUMN_SHARED, 
  COLUMN_EXACT_SHARED, 
  COLUMN_DESCRIPTION, 
  COLUMN_TAG,
  //[-]PPA		COLUMN_CONNECTION, 
  COLUMN_EMAIL, 
  //[-]PPA		COLUMN_VERSION, 
  //[-]PPA 		COLUMN_MODE, 
  COLUMN_HUBS, 
  COLUMN_SLOTS,
  //[-]PPA		COLUMN_UPLOAD_SPEED, 
  COLUMN_IP, 
#ifdef PPA_INCLUDE_DNS
  COLUMN_DNS, // !SMT!-IP
#endif
  //[-]PPA		COLUMN_PK, 
  COLUMN_LAST
};

struct UserTask : public Task {
  //UserTask(const UserPtr& u, Client* client_) : user(u), client(client_) { }
  UserTask(const UserPtr& u, Client* client_) : user(u), client(client_) { }

  // !SMT!-S
  virtual void beforeExecute() {
    if (user == ClientManager::getInstance()->getMe())
      identity = client->getMyIdentity();
    else
      ClientManager::getInstance()->getIdentity(user, identity);
  }

  UserPtr user;
  Identity identity;
  Client* client; // !SMT!-S: client required for distincting self on different hubs
};

struct MessageTask : public StringTask {
  MessageTask(const UserPtr& from_, const UserPtr& to_, const UserPtr& replyTo_, const string& m, bool annoying_) : StringTask(m), // !SMT!-S
    from(from_), to(to_), replyTo(replyTo_), annoying(annoying_) { }

  // !SMT!-S
  virtual void beforeExecute() {
    ClientManager::getInstance()->getIdentity(from, fromId);
    ClientManager::getInstance()->getIdentity(replyTo, replyToId);
  }

  UserPtr from;
  UserPtr to;
  UserPtr replyTo;
  bool annoying; // !SMT!-S
  Identity fromId;
  Identity replyToId;
};

class UserInfo : public UserInfoBase, public FastAlloc<UserInfo>, 
	 public ColumnBase< 9 > //[+]PPA
{
public:
	UserInfo(const UserTask& u) : UserInfoBase(u.user) {
		update(u.identity);
	};
	static int _compareItems(const UserInfo* a, const UserInfo* b, int col);
	uint8_t imageIndex() const { return WinUtil::getImage(identity); }

	void update(const Identity& identity);

	const string getNick() const { return identity.getNick(); }
	bool isHidden() const { return identity.isHidden(); }

	GETSET(Identity, identity, Identity);
	GETSET(uint8_t, flagimage, FlagImage);

        // !SMT!-S
        typedef HASH_MAP<UserPtr, UserInfo*, User::HashFunction> UserMapBase;
        typedef UserMapBase::const_iterator UserMapIter;
        class UserMap : public UserMapBase {
                public:
                UserInfo* findUser(const UserPtr& user) {
                        UserMapIter i = find(user);
                        return (i == end())? NULL : i->second;
                }
        };
};

class UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b) = 0;
	virtual bool isEmpty() { return false; }
	virtual ~UserInfoComparator() { }
};

class UserInfoReverseComparator: public UserInfoComparator {
private:
	auto_ptr<UserInfoComparator> m_original;
public:
	UserInfoReverseComparator(UserInfoComparator* original): m_original(original) { }
	virtual int compare(const UserInfo* a, const UserInfo* b) {
		return -m_original->compare(a, b);
	}
};

class UserInfoNickComparator: public UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b);
};

class UserInfoFavoriteNickComparator: public UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b);
};

class UserInfoShareComparator: public UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b);
};

class UserInfoDescriptionComparator: public UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b) {
		return UserInfo::_compareItems(a, b, COLUMN_DESCRIPTION);
	}
};

class UserInfoTagComparator: public UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b) {
		return UserInfo::_compareItems(a, b, COLUMN_TAG);
	}
};

class UserInfoEmailComparator: public UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b) {
		return UserInfo::_compareItems(a, b, COLUMN_EMAIL);
	}
};

class UserInfoHubsComparator: public UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b) {
		return UserInfo::_compareItems(a, b, COLUMN_HUBS);
	}
};

class UserInfoSlotsComparator: public UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b) {
		return UserInfo::_compareItems(a, b, COLUMN_SLOTS);
	}
};

class UserInfoIpComparator: public UserInfoComparator {
public:
	virtual int compare(const UserInfo* a, const UserInfo* b) {
		return UserInfo::_compareItems(a, b, COLUMN_IP);
	}
};

#endif //USERINFO_H
