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

#if !defined(UPLOAD_MANAGER_H)
#define UPLOAD_MANAGER_H

#include "UserConnection.h"
#include "Singleton.h"

//[-]PPA [Doxygen 1.5.1] #include "Client.h"
#include "ClientManager.h"
//[-]PPA [Doxygen 1.5.1] #include "ClientManagerListener.h"
#include "MerkleTree.h"
#include "FastAlloc.h"

class InputStream;

class Upload : public Transfer, public Flags {
public:
	enum Flags {
		FLAG_USER_LIST = 0x01,
		FLAG_TTH_LEAVES = 0x02,
		FLAG_ZUPLOAD = 0x04,
		FLAG_PARTIAL_LIST = 0x08,
		FLAG_PENDING_KICK = 0x10,
		FLAG_PARTIAL_SHARE = 0x20,
		FLAG_RESUMED = 0x40,
		FLAG_CHUNKED = 0x80
	};

	typedef Upload* Ptr;
	typedef FlyLinkVector<Ptr> List;
	typedef List::const_iterator Iter;
	
	Upload(UserConnection& conn);
	virtual ~Upload();
	
	virtual void getParams(const UserConnection& aSource, StringMap& params);
	
	GETSET(string, sourceFile, SourceFile);
	GETSET(InputStream*, stream, Stream);
};

class UploadManagerListener {
	friend class UploadQueueItem; 
public:
	virtual ~UploadManagerListener() { }
	template<int I>	struct X { enum { TYPE = I };  };
	
	typedef X<0> Complete;
	typedef X<1> Failed;
	typedef X<2> Starting;
	typedef X<3> Tick;
	typedef X<4> QueueAdd;
	typedef X<5> QueueRemove;
	typedef X<6> QueueItemRemove;
	typedef X<7> QueueUpdate;

	virtual void on(Starting, Upload*) throw() { }
	virtual void on(Tick, const Upload::List&) throw() { }
	virtual void on(Complete, Upload*) throw() { }
	virtual void on(Failed, Upload*, const string&) throw() { }
	virtual void on(QueueAdd, UploadQueueItem*) throw() { }
	virtual void on(QueueRemove, const UserPtr&) throw() { }
	virtual void on(QueueItemRemove, UploadQueueItem*) throw() { }
	virtual void on(QueueUpdate) throw() { }

};

class UploadQueueItem : public FastAlloc<UploadQueueItem>, public PointerBase,
	public ColumnBase< 12 >  //[+]PPA
{
public:
    UploadQueueItem(const UserPtr& u, const string& file, int64_t p, int64_t sz, uint32_t itime); // !SMT!-UI
	virtual ~UploadQueueItem() throw() { }
	typedef UploadQueueItem* Ptr;
	typedef vector<Ptr> List;
	typedef List::const_iterator Iter;
	typedef HASH_MAP<UserPtr, UploadQueueItem::List, User::HashFunction> UserMap;
	typedef UserMap::const_iterator UserMapIter;

	static int compareItems(const UploadQueueItem* a, const UploadQueueItem* b, int col) {
//+BugMaster: small optimization; fix; correct IP sorting
		switch(col) {
			case COLUMN_FILE: 
			case COLUMN_PATH: 
			case COLUMN_NICK: 
			case COLUMN_HUB: 
				   return Util::stricmp(a->getText(col), b->getText(col));
			case COLUMN_TRANSFERRED: return compare(a->pos, b->pos);
			case COLUMN_SIZE: return compare(a->size, b->size);
			case COLUMN_ADDED: return compare(a->iTime, b->iTime);
			case COLUMN_WAITING: return compare(a->iTime, b->iTime);
            case COLUMN_SLOTS: return compare(a->slots, b->slots); // !SMT!-UI
            case COLUMN_SHARE: return compare(a->share, b->share); // !SMT!-UI
			case COLUMN_IP: {
			uint32_t a_ip =0, b_ip =0;
			unsigned x1,x2,x3,x4;
				if(sscanf(Text::fromT(a->getText(col)).c_str(), "%d.%d.%d.%d", &x1,&x2,&x3,&x4) == 4)
					a_ip = (x1 << 24) + (x2 << 16) + (x3 << 8) + x4;
				if(sscanf(Text::fromT(b->getText(col)).c_str(), "%d.%d.%d.%d", &x1,&x2,&x3,&x4) == 4)
					b_ip = (x1 << 24) + (x2 << 16) + (x3 << 8) + x4;
				return compare(a_ip, b_ip);
			}
		}
		return Util::stricmp(a->getText(col), b->getText(col));
//-BugMaster: small optimization; fix; correct IP sorting
	}

	enum {
		COLUMN_FIRST,
		COLUMN_FILE = COLUMN_FIRST,
		COLUMN_PATH,
		COLUMN_NICK,
		COLUMN_HUB,
		COLUMN_TRANSFERRED,
		COLUMN_SIZE,
		COLUMN_ADDED,
		COLUMN_WAITING,
                COLUMN_LOCATION, // !SMT!-IP
                COLUMN_IP, // !SMT!-IP
#ifdef PPA_INCLUDE_DNS
                COLUMN_DNS, // !SMT!-IP
#endif
                COLUMN_SLOTS, // !SMT!-UI
                COLUMN_SHARE, // !SMT!-UI
		COLUMN_LAST
	};
		
	int imageIndex() const { return icon; }
	void update();

	UserPtr User;
	string File;
	int64_t pos;
	int64_t size;
	uint32_t iTime;
        uint64_t share; // !SMT!-UI
        int slots; // !SMT!-UI
        string ip, 
#ifdef PPA_INCLUDE_DNS
        dns, 
#endif
         country; // !SMT!-IP
        int flagImage; // !SMT!-IP
	int icon;
};

class UploadManager : private ClientManagerListener, private UserConnectionListener, public Speaker<UploadManagerListener>, private TimerManagerListener, public Singleton<UploadManager>
{
public:
	
	/** @return Number of uploads. */ 
	size_t getUploadCount() { Lock l(cs); return uploads.size(); }

	/**
	 * @remarks This is only used in the tray icons. Could be used in
	 * MainFrame too.
	 *
	 * @return Running average download speed in Bytes/s
	 */
	int64_t getRunningAverage();
	
	int getSlots() {
		int slots = 0;
		if (SETTING(HUB_SLOTS) * Client::getTotalCounts() <= SETTING(SLOTS)) {
			slots = SETTING(SLOTS);
		} else {
			slots = max(SETTING(HUB_SLOTS),0) * (Client::getTotalCounts());
		}
		return slots;
	}

	/** @return Number of free slots. */
	int getFreeSlots() { return max((getSlots() - running), 0); }
	
	/** @internal */
	int getFreeExtraSlots() { return max(SETTING(EXTRA_SLOTS) - getExtra(), 0); }
	
	/** @param aUser Reserve an upload slot for this user and connect. */
	void reserveSlot(const UserPtr& aUser, uint32_t aTime);
	void unreserveSlot(const UserPtr& aUser);
	void clearUserFiles(const UserPtr&);
	UploadQueueItem::UserMap getWaitingUsers();
	bool getFireballStatus() const { return isFireball; }
	bool getFileServerStatus() const { return isFileServer; }
        time_t getReservedSlotTime(const UserPtr& aUser) const; // !SMT!-S

        /** @internal */
	void addConnection(UserConnection::Ptr conn);

        void removeDelayUpload(const UserPtr& aUser) {
                Lock l(cs);
                for(Upload::List::iterator i = delayUploads.begin(); i != delayUploads.end(); ++i) {
                        Upload* up = *i;
                        if(aUser == up->getUser()) {
                                delayUploads.erase(i);
                                delete up;
                                break;
                        }
                }
        }

	void abortUpload(const string& aFile, bool waiting = true);
	
	// Upload throttling
	size_t throttleGetSlice();
	size_t throttleCycleTime();
	
	GETSET(int, running, Running);
	GETSET(int, extra, Extra);
	GETSET(uint64_t, lastGrant, LastGrant);

    void load(); // !SMT!-S
    void save(); // !SMT!-S
    bool isBanReply(const UserPtr& user); // !SMT!-S
private:
	Upload::List uploads;
	Upload::List delayUploads;
	mutable CriticalSection cs;
	
	typedef HASH_MAP<UserPtr, uint32_t, User::HashFunction> SlotMap;
	typedef SlotMap::iterator SlotIter;
	typedef SlotMap::const_iterator SlotIterC;
	SlotMap reservedSlots;
	
	UploadQueueItem::UserMap waitingUsers;
	void addFailedUpload(const UserPtr& User, const string& file, int64_t pos, int64_t size);
	
	void throttleZeroCounters();
	void throttleBytesTransferred(uint32_t i);
	void throttleSetup();
	bool mThrottleEnable;
	int    mBytesSpokenFor;
	size_t mBytesSent,
		   mUploadLimit,
		   mCycleTime,
		   mByteSlice;
	
	// Variables for Fireball and Fileserver detecting
	bool isFireball;
	bool isFileServer;
	uint32_t m_iHighSpeedStartTick;
	
	friend class Singleton<UploadManager>;
	UploadManager() throw();
	virtual ~UploadManager() throw();

        bool getAutoSlot();
        void removeConnection(UserConnection* aConn);
        void removeUpload(Upload* aUpload, bool delay = false);
        void logUpload(Upload* u);

        void testSlotTimeout(); // !SMT!-S

	// ClientManagerListener
	virtual void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser) throw();
	
	// TimerManagerListener
	virtual void on(Second, uint32_t aTick) throw();
	virtual void on(Minute, uint32_t aTick) throw();

	// UserConnectionListener
	virtual void on(BytesSent, UserConnection*, size_t, size_t) throw();
	virtual void on(Failed, UserConnection*, const string&) throw();
	virtual void on(Get, UserConnection*, const string&, int64_t) throw();
	virtual void on(Send, UserConnection*) throw();
	virtual void on(GetListLength, UserConnection* conn) throw();
	virtual void on(TransmitDone, UserConnection*) throw();
	
	virtual void on(AdcCommand::GET, UserConnection*, const AdcCommand&) throw();
	virtual void on(AdcCommand::GFI, UserConnection*, const AdcCommand&) throw();

	bool prepareFile(UserConnection& aSource, const string& aType, const string& aFile, int64_t aResume, int64_t& aBytes, bool listRecursive = false);

        // !SMT!-S
        struct banmsg_t
        {
                uint64_t tick;
                int slots, share, limit, min_slots, min_share, min_limit;
                bool same(const banmsg_t &a) const
				{
                        return ((slots ^ a.slots) | (share ^ a.share) | (limit ^ a.limit) |
                               (min_slots ^ a.min_slots) | (min_share ^ a.min_share) | (min_limit ^ a.min_limit)) == 0;
                }
        };
        typedef HASH_MAP<string,banmsg_t> BanMap;
        typedef BanMap::iterator BanMapIter;
        bool handleBan(UserConnection& aSource, bool forceBan, bool noChecks);
        bool hasUpload(UserConnection& aSource);
        static bool hasAutoBan(const UserPtr& aUser);
        BanMap lastBans;

        // !SMT!-S
        string getConfigFile() { return Util::getConfigPath() + "ExtraSlots.xml"; }
};

#endif // !defined(UPLOAD_MANAGER_H)

/**
 * @file
 * $Id: UploadManager.h,v 1.1.1.1 2007/09/27 13:21:19 alexey Exp $
 */
